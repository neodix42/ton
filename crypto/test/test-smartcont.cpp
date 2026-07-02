/*
    This file is part of TON Blockchain Library.

    TON Blockchain Library is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    TON Blockchain Library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with TON Blockchain Library.  If not, see <http://www.gnu.org/licenses/>.

    Copyright 2017-2020 Telegram Systems LLP
*/
#include <tuple>

#include "block/block-auto.h"
#include "block/block-parse.h"
#include "block/block.h"
#include "common/bigint.hpp"
#include "fift/Fift.h"
#include "fift/utils.h"
#include "fift/words.h"
#include "smc-envelope/GenericAccount.h"
#include "smc-envelope/ManualDns.h"
#include "smc-envelope/PaymentChannel.h"
#include "smc-envelope/SmartContract.h"
#include "smc-envelope/SmartContractCode.h"
#include "smc-envelope/WalletV3.h"
#include "smc-envelope/WalletV4.h"
#include "td/utils/PathView.h"
#include "td/utils/Random.h"
#include "td/utils/ScopeGuard.h"
#include "td/utils/StringBuilder.h"
#include "td/utils/Timer.h"
#include "td/utils/Variant.h"
#include "td/utils/base64.h"
#include "td/utils/crypto.h"
#include "td/utils/filesystem.h"
#include "td/utils/port/path.h"
#include "td/utils/tests.h"
#include "vm/dict.h"

#include "Ed25519.h"

std::string current_dir() {
  return td::PathView(td::realpath(__FILE__).move_as_ok()).parent_dir().str();
}

std::string load_source(std::string name) {
  return td::read_file_str(current_dir() + "../../crypto/" + name).move_as_ok();
}

td::Ref<vm::Cell> get_wallet_v3_source() {
  std::string code = R"ABCD(
SETCP0 DUP IFNOTRET // return if recv_internal
   DUP 85143 INT EQUAL OVER 78748 INT EQUAL OR IFJMP:<{ // "seqno" and "get_public_key" get-methods
     1 INT AND c4 PUSHCTR CTOS 32 LDU 32 LDU NIP 256 PLDU CONDSEL  // cnt or pubk
   }>
   INC 32 THROWIF	// fail unless recv_external
   9 PUSHPOW2 LDSLICEX DUP 32 LDU 32 LDU 32 LDU 	//  signature in_msg subwallet_id valid_until msg_seqno cs
   NOW s1 s3 XCHG LEQ 35 THROWIF	//  signature in_msg subwallet_id cs msg_seqno
   c4 PUSH CTOS 32 LDU 32 LDU 256 LDU ENDS	//  signature in_msg subwallet_id cs msg_seqno stored_seqno stored_subwallet public_key
   s3 s2 XCPU EQUAL 33 THROWIFNOT	//  signature in_msg subwallet_id cs public_key stored_seqno stored_subwallet
   s4 s4 XCPU EQUAL 34 THROWIFNOT	//  signature in_msg stored_subwallet cs public_key stored_seqno
   s0 s4 XCHG HASHSU	//  signature stored_seqno stored_subwallet cs public_key msg_hash
   s0 s5 s5 XC2PU	//  public_key stored_seqno stored_subwallet cs msg_hash signature public_key
   CHKSIGNU 35 THROWIFNOT	//  public_key stored_seqno stored_subwallet cs
   ACCEPT
   WHILE:<{
     DUP SREFS	//  public_key stored_seqno stored_subwallet cs _51
   }>DO<{	//  public_key stored_seqno stored_subwallet cs
     8 LDU LDREF s0 s2 XCHG	//  public_key stored_seqno stored_subwallet cs _56 mode
     SENDRAWMSG
   }>	//  public_key stored_seqno stored_subwallet cs
   ENDS SWAP INC	//  public_key stored_subwallet seqno'
   NEWC 32 STU 32 STU 256 STU ENDC c4 POP
)ABCD";
  return fift::compile_asm(code).move_as_ok();
}

TEST(Tonlib, WalletV3) {
  LOG(ERROR) << td::base64_encode(std_boc_serialize(get_wallet_v3_source()).move_as_ok());
  CHECK(get_wallet_v3_source()->get_hash() == ton::WalletV3::get_init_code(2)->get_hash());

  auto fift_output = fift::mem_run_fift(load_source("smartcont/new-wallet-v3.fif"), {"aba", "0", "239"}).move_as_ok();
  auto new_wallet_pk = fift_output.source_lookup.read_file("new-wallet.pk").move_as_ok().data;
  auto new_wallet_query = fift_output.source_lookup.read_file("new-wallet-query.boc").move_as_ok().data;
  auto new_wallet_addr = fift_output.source_lookup.read_file("new-wallet.addr").move_as_ok().data;

  td::Ed25519::PrivateKey priv_key{td::SecureString{new_wallet_pk}};
  auto pub_key = priv_key.get_public_key().move_as_ok();
  ton::WalletV3::InitData init_data;
  init_data.public_key = pub_key.as_octet_string();
  init_data.wallet_id = 239;
  auto wallet = ton::WalletV3::create(init_data, 2);
  ASSERT_EQ(239u, wallet->get_wallet_id().ok());
  ASSERT_EQ(0u, wallet->get_seqno().ok());

  auto address = wallet->get_address();
  CHECK(address.addr.as_slice() == td::Slice(new_wallet_addr).substr(0, 32));

  auto init_message = wallet->get_init_message(priv_key).move_as_ok();
  td::Ref<vm::Cell> ext_init_message = ton::GenericAccount::create_ext_message(
      address, ton::GenericAccount::get_init_state(wallet->get_state()), init_message);
  LOG(ERROR) << "-------";
  vm::load_cell_slice(ext_init_message).print_rec(std::cerr);
  LOG(ERROR) << "-------";
  vm::load_cell_slice(vm::std_boc_deserialize(new_wallet_query).move_as_ok()).print_rec(std::cerr);
  CHECK(vm::std_boc_deserialize(new_wallet_query).move_as_ok()->get_hash() == ext_init_message->get_hash());

  CHECK(wallet.write().send_external_message(init_message).success);

  fift_output.source_lookup.write_file("/main.fif", load_source("smartcont/wallet-v3.fif")).ensure();
  class ZeroOsTime : public fift::OsTime {
   public:
    td::uint32 now() override {
      return 0;
    }
  };
  fift_output.source_lookup.set_os_time(std::make_unique<ZeroOsTime>());
  auto dest = block::StdAddress::parse("Ef9Tj6fMJP+OqhAdhKXxq36DL+HYSzCc3+9O6UNzqsgPfYFX").move_as_ok();
  fift_output = fift::mem_run_fift(std::move(fift_output.source_lookup),
                                   {"aba", "new-wallet", "-C", "TESTv3",
                                    "Ef9Tj6fMJP+OqhAdhKXxq36DL+HYSzCc3+9O6UNzqsgPfYFX", "239", "1", "321"})
                    .move_as_ok();
  auto wallet_query = fift_output.source_lookup.read_file("wallet-query.boc").move_as_ok().data;

  ton::WalletV3::Gift gift;
  gift.destination = dest;
  gift.message = "TESTv3";
  gift.gramms = 321000000000ll;

  ASSERT_EQ(239u, wallet->get_wallet_id().ok());
  ASSERT_EQ(1u, wallet->get_seqno().ok());
  CHECK(priv_key.get_public_key().ok().as_octet_string() == wallet->get_public_key().ok().as_octet_string());
  CHECK(priv_key.get_public_key().ok().as_octet_string() ==
        ton::GenericAccount::get_public_key(*wallet).ok().as_octet_string());

  auto gift_message = ton::GenericAccount::create_ext_message(
      address, {}, wallet->make_a_gift_message(priv_key, 60, {gift}).move_as_ok());
  LOG(ERROR) << "-------";
  vm::load_cell_slice(gift_message).print_rec(std::cerr);
  LOG(ERROR) << "-------";
  vm::load_cell_slice(vm::std_boc_deserialize(wallet_query).move_as_ok()).print_rec(std::cerr);
  CHECK(vm::std_boc_deserialize(wallet_query).move_as_ok()->get_hash() == gift_message->get_hash());
}

template <class T>
void check_wallet_seqno(td::Ref<T> wallet, td::uint32 seqno) {
  ASSERT_EQ(seqno, wallet->get_seqno().ok());
}
void check_wallet_seqno(td::Ref<ton::WalletInterface> wallet, td::uint32 seqno) {
}
template <class T>
void check_wallet_state(td::Ref<T> wallet, td::uint32 seqno, td::uint32 wallet_id, td::Slice public_key) {
  ASSERT_EQ(wallet_id, wallet->get_wallet_id().ok());
  ASSERT_EQ(public_key, wallet->get_public_key().ok().as_octet_string().as_slice());
  check_wallet_seqno(wallet, seqno);
}

struct CreatedWallet {
  td::optional<td::Ed25519::PrivateKey> priv_key;
  block::StdAddress address;
  td::Ref<ton::WalletInterface> wallet;
};

template <class T>
class InitWallet {
 public:
  CreatedWallet operator()(int revision) const {
    ton::WalletInterface::DefaultInitData init_data;
    auto priv_key = td::Ed25519::generate_private_key().move_as_ok();
    auto pub_key = priv_key.get_public_key().move_as_ok();

    init_data.seqno = 0;
    init_data.wallet_id = 123;
    init_data.public_key = pub_key.as_octet_string();

    auto wallet = T::create(init_data, revision);
    auto address = wallet->get_address();
    check_wallet_state(wallet, 0, 123, init_data.public_key);
    CHECK(wallet.write().send_external_message(wallet->get_init_message(priv_key).move_as_ok()).success);

    CreatedWallet res;
    res.wallet = std::move(wallet);
    res.address = std::move(address);
    res.priv_key = std::move(priv_key);
    return res;
  }
};

template <class T>
void do_test_wallet(int revision) {
  auto res = InitWallet<T>()(revision);
  auto priv_key = res.priv_key.unwrap();
  auto address = std::move(res.address);
  auto iwallet = std::move(res.wallet);
  auto public_key = priv_key.get_public_key().move_as_ok().as_octet_string();

  check_wallet_state(iwallet, 1, 123, public_key);

  // lets send a lot of messages
  std::vector<ton::WalletInterface::Gift> gifts;
  for (size_t i = 0; i < iwallet->get_max_gifts_size(); i++) {
    ton::WalletInterface::Gift gift;
    gift.gramms = 1;
    gift.destination = address;
    gift.message = std::string(iwallet->get_max_message_size(), 'z');
    gifts.push_back(gift);
  }

  td::uint32 valid_until = 10000;
  auto send_gifts = iwallet->make_a_gift_message(priv_key, valid_until, gifts).move_as_ok();

  {
    auto cwallet = iwallet;
    CHECK(!cwallet.write()
               .send_external_message(send_gifts, ton::SmartContract::Args().set_now(valid_until + 1))
               .success);
  }
  //TODO: make wallet work (or not) with now == valid_until
  auto ans = iwallet.write().send_external_message(send_gifts, ton::SmartContract::Args().set_now(valid_until - 1));
  CHECK(ans.success);
  const size_t out_actions = static_cast<size_t>(ans.output_actions_count(ans.actions));
  CHECK(gifts.size() <= out_actions);
  check_wallet_state(iwallet, 2, 123, public_key);
}

template <class T>
void do_test_wallet() {
  for (auto revision : T::get_revisions()) {
    do_test_wallet<T>(revision);
  }
}

TEST(Tonlib, Wallet) {
  do_test_wallet<ton::WalletV3>();
  do_test_wallet<ton::WalletV4>();
}

class MapDns {
 public:
  using ManualDns = ton::ManualDns;
  struct Entry {
    std::string name;
    td::Bits256 category = td::Bits256::zero();
    std::string text;

    auto key() const {
      return std::tie(name, category);
    }
    bool operator<(const Entry& other) const {
      return key() < other.key();
    }
    bool operator==(const ton::DnsInterface::Entry& other) const {
      return key() == other.key() && other.data.type == ManualDns::EntryData::Type::Text &&
             other.data.data.get<ManualDns::EntryDataText>().text == text;
    }
    bool operator==(const Entry& other) const {
      return key() == other.key() && text == other.text;
    }
    friend td::StringBuilder& operator<<(td::StringBuilder& sb, const Entry& entry) {
      return sb << "[" << entry.name << ":" << entry.category.to_hex() << ":" << entry.text << "]";
    }
  };
  struct Action {
    std::string name;
    td::Bits256 category = td::Bits256::zero();
    td::optional<std::string> text;

    bool does_create_category() const {
      CHECK(!name.empty());
      CHECK(!category.is_zero());
      return static_cast<bool>(text);
    }
    bool does_change_empty() const {
      CHECK(!name.empty());
      CHECK(!category.is_zero());
      return static_cast<bool>(text) && !text.value().empty();
    }
    void make_non_empty() {
      CHECK(!name.empty());
      CHECK(!category.is_zero());
      if (!text) {
        text = "";
      }
    }
    friend td::StringBuilder& operator<<(td::StringBuilder& sb, const Action& entry) {
      return sb << "[" << entry.name << ":" << entry.category.to_hex() << ":"
                << (entry.text ? entry.text.value() : "<empty>") << "]";
    }
  };
  void update(td::Span<Action> actions) {
    for (auto& action : actions) {
      do_update(action);
    }
  }
  using CombinedActions = ton::ManualDns::CombinedActions<Action>;
  void update_combined(td::Span<Action> actions) {
    LOG(ERROR) << "BEGIN";
    LOG(ERROR) << td::format::as_array(actions);
    auto combined_actions = ton::ManualDns::combine_actions(actions);
    for (auto& c : combined_actions) {
      LOG(ERROR) << c.name << ":" << c.category.to_hex();
      if (c.actions) {
        LOG(ERROR) << td::format::as_array(c.actions.value());
      }
    }
    LOG(ERROR) << "END";
    for (auto& combined_action : combined_actions) {
      do_update(combined_action);
    }
  }

  std::vector<Entry> resolve(td::Slice name, td::Bits256 category) {
    std::vector<Entry> res;
    if (name.empty()) {
      for (auto& a : entries_) {
        for (auto& b : a.second) {
          res.push_back({a.first, b.first, b.second});
        }
      }
    } else {
      auto it = entries_.find(name);
      while (it == entries_.end()) {
        auto sz = name.find('.');
        category = ton::DNS_NEXT_RESOLVER_CATEGORY;
        if (sz != td::Slice::npos) {
          name = name.substr(sz + 1);
        } else {
          break;
        }
        it = entries_.find(name);
      }
      if (it != entries_.end()) {
        for (auto& b : it->second) {
          if (category.is_zero() || category == b.first) {
            res.push_back({name.str(), b.first, b.second});
          }
        }
      }
    }

    std::sort(res.begin(), res.end());
    return res;
  }

 private:
  std::map<std::string, std::map<td::Bits256, std::string>, std::less<>> entries_;
  void do_update(const Action& action) {
    if (action.name.empty()) {
      entries_.clear();
      return;
    }
    if (action.category.is_zero()) {
      entries_.erase(action.name);
      return;
    }
    if (action.text) {
      if (action.text.value().empty()) {
        entries_[action.name].erase(action.category);
      } else {
        entries_[action.name][action.category] = action.text.value();
      }
    } else {
      auto it = entries_.find(action.name);
      if (it != entries_.end()) {
        it->second.erase(action.category);
      }
    }
  }

  void do_update(const CombinedActions& actions) {
    if (actions.name.empty()) {
      entries_.clear();
      LOG(ERROR) << "CLEAR";
      if (!actions.actions) {
        return;
      }
      for (auto& action : actions.actions.value()) {
        CHECK(!action.name.empty());
        CHECK(!action.category.is_zero());
        CHECK(action.text);
        if (action.text.value().empty()) {
          entries_[action.name];
        } else {
          entries_[action.name][action.category] = action.text.value();
        }
      }
      return;
    }
    if (actions.category.is_zero()) {
      entries_.erase(actions.name);
      LOG(ERROR) << "CLEAR " << actions.name;
      if (!actions.actions) {
        return;
      }
      entries_[actions.name];
      for (auto& action : actions.actions.value()) {
        CHECK(action.name == actions.name);
        CHECK(!action.category.is_zero());
        CHECK(action.text);
        if (action.text.value().empty()) {
          entries_[action.name];
        } else {
          entries_[action.name][action.category] = action.text.value();
        }
      }
      return;
    }
    CHECK(actions.actions);
    CHECK(actions.actions.value().size() == 1);
    for (auto& action : actions.actions.value()) {
      CHECK(action.name == actions.name);
      CHECK(!action.category.is_zero());
      if (action.text) {
        if (action.text.value().empty()) {
          entries_[action.name].erase(action.category);
        } else {
          entries_[action.name][action.category] = action.text.value();
        }
      } else {
        auto it = entries_.find(action.name);
        if (it != entries_.end()) {
          it->second.erase(action.category);
        }
      }
    }
  }
};

class CheckedDns {
 public:
  explicit CheckedDns(bool check_smc = true, bool check_combine = true) {
    if (check_smc) {
      key_ = td::Ed25519::generate_private_key().move_as_ok();
      dns_ = ManualDns::create(ManualDns::create_init_data_fast(key_.value().get_public_key().move_as_ok(), 123), -1);
    }
    if (check_combine) {
      combined_map_dns_ = MapDns();
    }
  }
  using Action = MapDns::Action;
  using Entry = MapDns::Entry;
  void update(td::Span<Action> entries) {
    if (dns_.not_null()) {
      auto smc_actions = td::transform(entries, [](auto& entry) {
        ton::DnsInterface::Action action;
        action.name = entry.name;
        action.category = entry.category;
        if (entry.text) {
          if (entry.text.value().empty()) {
            action.data = td::Ref<vm::Cell>();
          } else {
            action.data = ManualDns::EntryData::text(entry.text.value()).as_cell().move_as_ok();
          }
        }
        return action;
      });
      auto query = dns_->create_update_query(key_.value(), smc_actions, query_id_++).move_as_ok();
      CHECK(dns_.write().send_external_message(std::move(query)).code == 0);
    }
    map_dns_.update(entries);
    if (combined_map_dns_) {
      combined_map_dns_.value().update_combined(entries);
    }
  }
  void update(const Action& action) {
    return update(td::span_one(action));
  }

  std::vector<Entry> resolve(td::Slice name, td::Bits256 category) {
    LOG(ERROR) << "RESOLVE: " << name << " " << category.to_hex();
    auto res = map_dns_.resolve(name, category);
    LOG(ERROR) << td::format::as_array(res);

    if (dns_.not_null()) {
      auto other_res = dns_->resolve(name, category).move_as_ok();

      std::sort(other_res.begin(), other_res.end());
      if (res.size() != other_res.size()) {
        LOG(ERROR) << td::format::as_array(res);
        LOG(FATAL) << td::format::as_array(other_res);
      }
      for (size_t i = 0; i < res.size(); i++) {
        if (!(res[i] == other_res[i])) {
          LOG(ERROR) << td::format::as_array(res);
          LOG(FATAL) << td::format::as_array(other_res);
        }
      }
    }
    if (combined_map_dns_) {
      auto other_res = combined_map_dns_.value().resolve(name, category);

      std::sort(other_res.begin(), other_res.end());
      if (res.size() != other_res.size()) {
        LOG(ERROR) << td::format::as_array(res);
        LOG(FATAL) << td::format::as_array(other_res);
      }
      for (size_t i = 0; i < res.size(); i++) {
        if (!(res[i] == other_res[i])) {
          LOG(ERROR) << td::format::as_array(res);
          LOG(FATAL) << td::format::as_array(other_res);
        }
      }
    }

    return res;
  }

 private:
  using ManualDns = ton::ManualDns;
  td::optional<td::Ed25519::PrivateKey> key_;
  td::Ref<ManualDns> dns_;
  td::uint32 query_id_ = 1;  // Query id serve as "valid until", but in tests now() == 0

  MapDns map_dns_;
  td::optional<MapDns> combined_map_dns_;

  void do_update_smc(const Action& entry) {
    LOG(ERROR) << td::format::escaped(ManualDns::encode_name(entry.name));
    ton::DnsInterface::Action action;
    action.name = entry.name;
    action.category = entry.category;
    action.data = ManualDns::EntryData::text(entry.text.value()).as_cell().move_as_ok();
  }
};

static td::Bits256 intToCat(td::uint32 x) {
  auto y = td::make_refint(x);
  td::Bits256 cat;
  y->export_bytes(cat.data(), 32, false);
  return cat;
}

void do_dns_test(CheckedDns&& dns) {
  using Action = CheckedDns::Action;
  std::vector<Action> actions;

  td::Random::Xorshift128plus rnd(123);

  auto gen_name = [&] {
    auto cnt = rnd.fast(1, 2);
    std::string res;
    for (int i = 0; i < cnt; i++) {
      if (i != 0) {
        res += '.';
      }
      auto len = rnd.fast(1, 1);
      for (int j = 0; j < len; j++) {
        res += static_cast<char>(rnd.fast('a', 'b'));
      }
    }
    return res;
  };
  auto gen_text = [&] {
    std::string res;
    int len = 5;
    for (int j = 0; j < len; j++) {
      res += static_cast<char>(rnd.fast('a', 'b'));
    }
    return res;
  };

  auto gen_action = [&] {
    Action action;
    if (rnd.fast(0, 1000) == 0) {
      return action;
    }
    action.name = gen_name();
    if (rnd.fast(0, 20) == 0) {
      return action;
    }
    action.category = intToCat(rnd.fast(1, 5));
    if (rnd.fast(0, 4) == 0) {
      return action;
    }
    if (rnd.fast(0, 4) == 0) {
      action.text = "";
      return action;
    }
    action.text = gen_text();
    return action;
  };

  SET_VERBOSITY_LEVEL(VERBOSITY_NAME(ERROR));
  for (int i = 0; i < 100000; i++) {
    actions.push_back(gen_action());
    if (rnd.fast(0, 10) == 0) {
      dns.update(actions);
      actions.clear();
    }
    auto name = gen_name();
    dns.resolve(name, intToCat(rnd.fast(0, 5)));
  }
};

TEST(Smartcont, DnsManual) {
  using ManualDns = ton::ManualDns;
  auto test_entry_data = [](auto&& entry_data) {
    auto cell = entry_data.as_cell().move_as_ok();
    auto cs = vm::load_cell_slice(cell);
    auto new_entry_data = ManualDns::EntryData::from_cellslice(cs).move_as_ok();
    ASSERT_EQ(entry_data, new_entry_data);
  };
  test_entry_data(ManualDns::EntryData::text("abcd"));
  test_entry_data(ManualDns::EntryData::adnl_address(ton::Bits256{}));

  CHECK(td::Slice("a\0b\0") == ManualDns::encode_name("b.a"));
  CHECK(td::Slice("a\0b\0") == ManualDns::encode_name(".b.a"));
  ASSERT_EQ(".b.a", ManualDns::decode_name("a\0b\0"));
  ASSERT_EQ("b.a", ManualDns::decode_name("a\0b"));
  ASSERT_EQ("", ManualDns::decode_name(""));

  auto key = td::Ed25519::generate_private_key().move_as_ok();

  auto manual = ManualDns::create(ManualDns::create_init_data_fast(key.get_public_key().move_as_ok(), 123), -1);
  CHECK(manual->get_wallet_id().move_as_ok() == 123);
  auto init_query = manual->create_init_query(key).move_as_ok();
  LOG(ERROR) << "A";
  CHECK(manual.write().send_external_message(init_query).code == 0);
  LOG(ERROR) << "B";
  CHECK(manual.write().send_external_message(init_query).code != 0);

  auto value = vm::CellBuilder().store_bytes("hello world").finalize();
  auto set_query =
      manual
          ->sign(key, manual->prepare(manual->create_set_value_unsigned(intToCat(1), "a\0b\0", value).move_as_ok(), 1)
                          .move_as_ok())
          .move_as_ok();
  CHECK(manual.write().send_external_message(set_query).code == 0);

  auto res = manual->run_get_method(
      "dnsresolve", {vm::load_cell_slice_ref(vm::CellBuilder().store_bytes("a\0b\0").finalize()), td::make_refint(1)});
  CHECK(res.code == 0);
  CHECK(res.stack.write().pop_cell()->get_hash() == value->get_hash());

  CheckedDns dns;
  dns.update(CheckedDns::Action{"a.b.c", intToCat(1), "hello"});
  CHECK(dns.resolve("a.b.c", intToCat(1)).at(0).text == "hello");
  dns.resolve("a", intToCat(1));
  dns.resolve("a.b", intToCat(1));
  CHECK(dns.resolve("a.b.c", intToCat(2)).empty());
  dns.update(CheckedDns::Action{"a.b.c", intToCat(2), "test"});
  CHECK(dns.resolve("a.b.c", intToCat(2)).at(0).text == "test");
  dns.resolve("a.b.c", intToCat(1));
  dns.resolve("a.b.c", intToCat(2));
  LOG(ERROR) << "Test zero category";
  dns.resolve("a.b.c", intToCat(0));
  dns.update(CheckedDns::Action{"", intToCat(0), ""});
  CHECK(dns.resolve("a.b.c", intToCat(2)).empty());

  LOG(ERROR) << "Test multipe update";
  {
    CheckedDns::Action e[4] = {
        CheckedDns::Action{"", intToCat(0), ""}, CheckedDns::Action{"a.b.c", intToCat(1), "hello"},
        CheckedDns::Action{"a.b.c", intToCat(2), "world"}, CheckedDns::Action{"x.y.z", intToCat(3), "abc"}};
    dns.update(td::span(e, 4));
  }
  dns.resolve("a.b.c", intToCat(1));
  dns.resolve("a.b.c", intToCat(2));
  dns.resolve("x.y.z", intToCat(3));

  dns.update(td::span_one(CheckedDns::Action{"x.y.z", intToCat(0), ""}));

  dns.resolve("a.b.c", intToCat(1));
  dns.resolve("a.b.c", intToCat(2));
  dns.resolve("x.y.z", intToCat(3));

  {
    CheckedDns::Action e[3] = {CheckedDns::Action{"x.y.z", intToCat(0), ""},
                               CheckedDns::Action{"x.y.z", intToCat(1), "xxx"},
                               CheckedDns::Action{"x.y.z", intToCat(2), "yyy"}};
    dns.update(td::span(e, 3));
  }
  dns.resolve("a.b.c", intToCat(1));
  dns.resolve("a.b.c", intToCat(2));
  dns.resolve("x.y.z", intToCat(1));
  dns.resolve("x.y.z", intToCat(2));
  dns.resolve("x.y.z", intToCat(3));

  {
    auto actions_ext =
        ton::ManualDns::parse("delete.name one\nset one 1 TEXT:one\ndelete.name two\nset two 2 TEXT:two").move_as_ok();

    auto actions = td::transform(actions_ext, [](auto& action) {
      td::optional<std::string> data;
      if (action.data) {
        data = action.data.value().data.template get<ton::ManualDns::EntryDataText>().text;
      }
      return CheckedDns::Action{action.name, action.category, std::move(data)};
    });

    dns.update(actions);
  }
  dns.resolve("one", intToCat(1));
  dns.resolve("two", intToCat(2));

  // TODO: rethink semantic of creating an empty dictionary
  do_dns_test(CheckedDns(true, true));
}

using namespace ton::pchan;

template <class T>
struct ValidateState {
  T& self() {
    return static_cast<T&>(*this);
  }

  void init(td::Ref<vm::Cell> state) {
    state_ = state;
    block::gen::ChanData::Record data_rec;
    if (!tlb::unpack_cell(state, data_rec)) {
      on_fatal_error(td::Status::Error("Expected Data"));
      return;
    }
    if (!tlb::unpack_cell(data_rec.state, self().rec)) {
      on_fatal_error(td::Status::Error("Expected StatePayout"));
      return;
    }
    CHECK(self().rec.A.not_null());
  }

  T& expect_grams(td::Ref<vm::CellSlice> cs, td::uint64 expected, td::Slice name) {
    if (has_fatal_error_) {
      return self();
    }
    td::RefInt256 got;
    CHECK(cs.not_null());
    CHECK(block::tlb::t_Grams.as_integer_to(cs, got));
    if (got->cmp(expected) != 0) {
      on_error(td::Status::Error(PSLICE() << name << ": expected " << expected << ", got " << got->to_dec_string()));
    }
    return self();
  }
  template <class S>
  T& expect_eq(S a, S expected, td::Slice name) {
    if (has_fatal_error_) {
      return self();
    }
    if (!(a == expected)) {
      on_error(td::Status::Error(PSLICE() << name << ": expected " << expected << ", got " << a));
    }
    return self();
  }

  td::Status finish() {
    if (errors_.empty()) {
      return td::Status::OK();
    }
    std::stringstream ss;
    block::gen::t_ChanData.print_ref(ss, state_);
    td::StringBuilder sb;
    for (auto& error : errors_) {
      sb << error << "\n";
    }
    sb << ss.str();
    return td::Status::Error(sb.as_cslice());
  }

  void on_fatal_error(td::Status error) {
    CHECK(!has_fatal_error_);
    has_fatal_error_ = true;
    on_error(std::move(error));
  }
  void on_error(td::Status error) {
    CHECK(error.is_error());
    errors_.push_back(std::move(error));
  }

 public:
  td::Ref<vm::Cell> state_;
  bool has_fatal_error_{false};
  std::vector<td::Status> errors_;
};

struct ValidateStatePayout : public ValidateState<ValidateStatePayout> {
  ValidateStatePayout& expect_A(td::uint64 a) {
    expect_grams(rec.A, a, "A");
    return *this;
  }
  ValidateStatePayout& expect_B(td::uint64 b) {
    expect_grams(rec.B, b, "B");
    return *this;
  }

  ValidateStatePayout(td::Ref<vm::Cell> state) {
    init(std::move(state));
  }

  block::gen::ChanState::Record_chan_state_payout rec;
};

struct ValidateStateInit : public ValidateState<ValidateStateInit> {
  ValidateStateInit& expect_A(td::uint64 a) {
    expect_grams(rec.A, a, "A");
    return *this;
  }
  ValidateStateInit& expect_B(td::uint64 b) {
    expect_grams(rec.B, b, "B");
    return *this;
  }
  ValidateStateInit& expect_min_A(td::uint64 a) {
    expect_grams(rec.min_A, a, "min_A");
    return *this;
  }
  ValidateStateInit& expect_min_B(td::uint64 b) {
    expect_grams(rec.min_B, b, "min_B");
    return *this;
  }
  ValidateStateInit& expect_expire_at(td::uint32 b) {
    expect_eq(rec.expire_at, b, "expire_at");
    return *this;
  }
  ValidateStateInit& expect_signed_A(bool x) {
    expect_eq(rec.signed_A, x, "signed_A");
    return *this;
  }
  ValidateStateInit& expect_signed_B(bool x) {
    expect_eq(rec.signed_B, x, "signed_B");
    return *this;
  }

  ValidateStateInit(td::Ref<vm::Cell> state) {
    init(std::move(state));
  }

  block::gen::ChanState::Record_chan_state_init rec;
};

struct ValidateStateClose : public ValidateState<ValidateStateClose> {
  ValidateStateClose& expect_A(td::uint64 a) {
    expect_grams(rec.A, a, "A");
    return *this;
  }
  ValidateStateClose& expect_B(td::uint64 b) {
    expect_grams(rec.B, b, "B");
    return *this;
  }
  ValidateStateClose& expect_promise_A(td::uint64 a) {
    expect_grams(rec.promise_A, a, "promise_A");
    return *this;
  }
  ValidateStateClose& expect_promise_B(td::uint64 b) {
    expect_grams(rec.promise_B, b, "promise_B");
    return *this;
  }
  ValidateStateClose& expect_expire_at(td::uint32 b) {
    expect_eq(rec.expire_at, b, "expire_at");
    return *this;
  }
  ValidateStateClose& expect_signed_A(bool x) {
    expect_eq(rec.signed_A, x, "signed_A");
    return *this;
  }
  ValidateStateClose& expect_signed_B(bool x) {
    expect_eq(rec.signed_B, x, "signed_B");
    return *this;
  }

  ValidateStateClose(td::Ref<vm::Cell> state) {
    init(std::move(state));
  }

  block::gen::ChanState::Record_chan_state_close rec;
};

// config$_ initTimeout:int exitTimeout:int a_key:int256 b_key:int256 a_addr b_addr channel_id:int256 = Config;
TEST(Smarcont, Channel) {
  auto code = ton::SmartContractCode::get_code(ton::SmartContractCode::PaymentChannel);
  Config config;
  auto a_pkey = td::Ed25519::generate_private_key().move_as_ok();
  auto b_pkey = td::Ed25519::generate_private_key().move_as_ok();
  config.init_timeout = 20;
  config.close_timeout = 40;
  auto dest = block::StdAddress::parse("Ef9Tj6fMJP+OqhAdhKXxq36DL+HYSzCc3+9O6UNzqsgPfYFX").move_as_ok();
  config.a_addr = dest;
  config.b_addr = dest;
  config.a_key = a_pkey.get_public_key().ok().as_octet_string();
  config.b_key = b_pkey.get_public_key().ok().as_octet_string();
  config.channel_id = 123;

  Data data;
  data.config = config.serialize();
  data.state = data.init_state();
  auto data_cell = data.serialize();

  auto channel = ton::SmartContract::create(ton::SmartContract::State{code, data_cell});
  ValidateStateInit(channel->get_state().data)
      .expect_A(0)
      .expect_B(0)
      .expect_min_A(0)
      .expect_min_B(0)
      .expect_signed_A(false)
      .expect_signed_B(false)
      .expect_expire_at(0)
      .finish()
      .ensure();

  enum err {
    ok = 0,
    wrong_a_signature = 31,
    wrong_b_signature,
    msg_value_too_small,
    replay_protection,
    no_timeout,
    expected_init,
    expected_close,
    no_promise_signature,
    wrong_channel_id
  };

#define expect_code(description, expected_code, e)                                                            \
  {                                                                                                           \
    auto res = e;                                                                                             \
    LOG_IF(FATAL, expected_code != res.code) << " res.code=" << res.code << " " << description << "\n" << #e; \
  }
#define expect_ok(description, e) expect_code(description, 0, e)

  expect_code("Trying to invoke a timeout while channel is empty", no_timeout,
              channel.write().send_external_message(MsgTimeoutBuilder().finalize(),
                                                    ton::SmartContract::Args().set_now(1000000)));

  expect_code("External init message with no signatures", replay_protection,
              channel.write().send_external_message(MsgInitBuilder().channel_id(config.channel_id).finalize()));
  expect_code("Internal init message with not enough value", msg_value_too_small,
              channel.write().send_internal_message(
                  MsgInitBuilder().channel_id(config.channel_id).inc_A(1000).min_B(2000).with_a_key(&a_pkey).finalize(),
                  ton::SmartContract::Args().set_amount(100)));
  expect_code(
      "Internal init message with wrong channel_id", wrong_channel_id,
      channel.write().send_internal_message(MsgInitBuilder().inc_A(1000).min_B(2000).with_a_key(&a_pkey).finalize(),
                                            ton::SmartContract::Args().set_amount(1000)));
  expect_ok("A init with (inc_A = 1000, min_A = 1, min_B = 2000)",
            channel.write().send_internal_message(MsgInitBuilder()
                                                      .channel_id(config.channel_id)
                                                      .inc_A(1000)
                                                      .min_A(1)
                                                      .min_B(2000)
                                                      .with_a_key(&a_pkey)
                                                      .finalize(),
                                                  ton::SmartContract::Args().set_amount(1000)));
  ValidateStateInit(channel->get_state().data)
      .expect_A(1000)
      .expect_B(0)
      .expect_min_A(1)
      .expect_min_B(2000)
      .expect_signed_A(true)
      .expect_signed_B(false)
      .expect_expire_at(config.init_timeout)
      .finish()
      .ensure();

  expect_code("Repeated init of A init with (inc_A = 100, min_B = 5000). Must be ignored", replay_protection,
              channel.write().send_internal_message(
                  MsgInitBuilder().channel_id(config.channel_id).inc_A(100).min_B(5000).with_a_key(&a_pkey).finalize(),
                  ton::SmartContract::Args().set_amount(1000)));
  expect_code(
      "Trying to invoke a timeout too early", no_timeout,
      channel.write().send_external_message(MsgTimeoutBuilder().finalize(), ton::SmartContract::Args().set_now(0)));

  {
    auto channel_copy = channel;
    expect_ok("Invoke a timeout", channel_copy.write().send_external_message(MsgTimeoutBuilder().finalize(),
                                                                             ton::SmartContract::Args().set_now(21)));
    ValidateStatePayout(channel_copy->get_state().data).expect_A(1000).expect_B(0).finish().ensure();
  }
  {
    auto channel_copy = channel;
    expect_ok("B init with inc_B < min_B. Leads to immediate payout",
              channel_copy.write().send_internal_message(
                  MsgInitBuilder().channel_id(config.channel_id).inc_B(1500).with_b_key(&b_pkey).finalize(),
                  ton::SmartContract::Args().set_amount(1500)));
    ValidateStatePayout(channel_copy->get_state().data).expect_A(1000).expect_B(1500).finish().ensure();
  }

  expect_ok("B init with (inc_B = 2000, min_A = 1, min_A = 1000)",
            channel.write().send_internal_message(
                MsgInitBuilder().channel_id(config.channel_id).inc_B(2000).min_A(1000).with_b_key(&b_pkey).finalize(),
                ton::SmartContract::Args().set_amount(2000)));
  ValidateStateClose(channel->get_state().data)
      .expect_A(1000)
      .expect_B(2000)
      .expect_promise_A(0)
      .expect_promise_B(0)
      .expect_signed_A(false)
      .expect_signed_B(false)
      .expect_expire_at(0)
      .finish()
      .ensure();

  {
    auto channel_copy = channel;
    expect_ok("A&B send Promise(1000000, 1000000 + 10) signed by nobody",
              channel_copy.write().send_external_message(MsgCloseBuilder()
                                                             .signed_promise(SignedPromiseBuilder()
                                                                                 .promise_A(1000000)
                                                                                 .promise_B(1000000 + 10)
                                                                                 .channel_id(config.channel_id)
                                                                                 .finalize())
                                                             .with_a_key(&a_pkey)
                                                             .with_b_key(&b_pkey)
                                                             .finalize(),
                                                         ton::SmartContract::Args().set_now(21)));
    ValidateStatePayout(channel_copy->get_state().data).expect_A(1000 + 10).expect_B(2000 - 10).finish().ensure();
  }
  {
    auto channel_copy = channel;
    expect_ok("A&B send Promise(1000000, 1000000 + 10) signed by A",
              channel_copy.write().send_external_message(MsgCloseBuilder()
                                                             .signed_promise(SignedPromiseBuilder()
                                                                                 .promise_A(1000000)
                                                                                 .promise_B(1000000 + 10)
                                                                                 .with_key(&a_pkey)
                                                                                 .channel_id(config.channel_id)
                                                                                 .finalize())
                                                             .with_a_key(&a_pkey)
                                                             .with_b_key(&b_pkey)
                                                             .finalize(),
                                                         ton::SmartContract::Args().set_now(21)));
    ValidateStatePayout(channel_copy->get_state().data).expect_A(1000 + 10).expect_B(2000 - 10).finish().ensure();
  }

  expect_code(
      "A sends Promise(1000000, 0) signed by A", wrong_b_signature,
      channel.write().send_external_message(
          MsgCloseBuilder()
              .signed_promise(
                  SignedPromiseBuilder().promise_A(1000000).with_key(&a_pkey).channel_id(config.channel_id).finalize())
              .with_a_key(&a_pkey)
              .finalize(),
          ton::SmartContract::Args().set_now(21)));
  expect_code(
      "B sends Promise(1000000, 0) signed by B", wrong_a_signature,
      channel.write().send_external_message(
          MsgCloseBuilder()
              .signed_promise(
                  SignedPromiseBuilder().promise_A(1000000).with_key(&b_pkey).channel_id(config.channel_id).finalize())
              .with_b_key(&b_pkey)
              .finalize(),
          ton::SmartContract::Args().set_now(21)));
  expect_code("B sends Promise(1000000, 0) signed by A with wrong channel_id", wrong_channel_id,
              channel.write().send_external_message(MsgCloseBuilder()
                                                        .signed_promise(SignedPromiseBuilder()
                                                                            .promise_A(1000000)
                                                                            .with_key(&a_pkey)
                                                                            .channel_id(config.channel_id + 1)
                                                                            .finalize())
                                                        .with_b_key(&b_pkey)
                                                        .finalize(),
                                                    ton::SmartContract::Args().set_now(21)));
  expect_code(
      "B sends unsigned Promise(1000000, 0)", no_promise_signature,
      channel.write().send_external_message(
          MsgCloseBuilder()
              .signed_promise(SignedPromiseBuilder().promise_A(1000000).channel_id(config.channel_id).finalize())
              .with_b_key(&b_pkey)
              .finalize(),
          ton::SmartContract::Args().set_now(21)));

  expect_ok(
      "B sends Promise(1000000, 0) signed by A",
      channel.write().send_external_message(
          MsgCloseBuilder()
              .signed_promise(
                  SignedPromiseBuilder().promise_A(1000000).with_key(&a_pkey).channel_id(config.channel_id).finalize())
              .with_b_key(&b_pkey)
              .finalize(),
          ton::SmartContract::Args().set_now(21)));
  ValidateStateClose(channel->get_state().data)
      .expect_A(1000)
      .expect_B(2000)
      .expect_promise_A(1000000)
      .expect_promise_B(0)
      .expect_signed_A(false)
      .expect_signed_B(true)
      .expect_expire_at(21 + config.close_timeout)
      .finish()
      .ensure();

  expect_ok("B sends Promise(0, 1000000 + 10) signed by A",
            channel.write().send_external_message(MsgCloseBuilder()
                                                      .signed_promise(SignedPromiseBuilder()
                                                                          .promise_B(1000000 + 10)
                                                                          .with_key(&b_pkey)
                                                                          .channel_id(config.channel_id)
                                                                          .finalize())
                                                      .with_a_key(&a_pkey)
                                                      .finalize(),
                                                  ton::SmartContract::Args().set_now(21)));
  ValidateStatePayout(channel->get_state().data).expect_A(1000 + 10).expect_B(2000 - 10).finish().ensure();
#undef expect_ok
#undef expect_code
}

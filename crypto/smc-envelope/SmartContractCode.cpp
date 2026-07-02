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
#include <map>

#include "td/utils/base64.h"
#include "vm/boc.h"

#include "SmartContractCode.h"

namespace ton {
namespace {
// WALLET_REVISION = 2;
// WALLET2_REVISION = 2;
// WALLET3_REVISION = 2;
// WALLET4_REVISION = 2;
// DNS_REVISION = 1;
const auto& get_map() {
  static auto map = [] {
    std::map<std::string, td::Ref<vm::Cell>, std::less<>> map;
    auto with_tvm_code = [&](auto name, td::Slice code_str) {
      map[name] = vm::std_boc_deserialize(td::base64_decode(code_str).move_as_ok()).move_as_ok();
    };
#include "smartcont/auto/dns-manual-code.cpp"
#include "smartcont/auto/payment-channel-code.cpp"
#include "smartcont/auto/wallet-code.cpp"

    with_tvm_code("wallet3-r1",
                  "te6ccgEBAQEAYgAAwP8AIN0gggFMl7qXMO1E0NcLH+Ck8mCDCNcYINMf0x/TH/gjE7vyY+1E0NMf0x/T/"
                  "9FRMrryoVFEuvKiBPkBVBBV+RDyo/gAkyDXSpbTB9QC+wDo0QGkyMsfyx/L/8ntVA==");
    with_tvm_code("wallet3-r2",
                  "te6ccgEBAQEAcQAA3v8AIN0gggFMl7ohggEznLqxn3Gw7UTQ0x/THzHXC//jBOCk8mCDCNcYINMf0x/TH/gjE7vyY+1E0NMf0x/"
                  "T/9FRMrryoVFEuvKiBPkBVBBV+RDyo/gAkyDXSpbTB9QC+wDo0QGkyMsfyx/L/8ntVA==");
    with_tvm_code(
        "dns-manual-r1",
        "te6ccgECGAEAAtAAART/APSkE/S88sgLAQIBIAIDAgFIBAUC7PLbPAWDCNcYIPkBAdMf0z/"
        "4I6ofUyC58mNTKoBA9A5voTHyYFKUuvKiVBNG+RDyo/gAItcLBcAzmDQBdtch0/"
        "8wjoVa2zxAA+"
        "IDgyWhyEAHgED0Q44aIIBA9JZvpTJREJQwUwe53iCTMzUBkjIw4rPmNVUD8AQREgICxQYHAgEgDA0CAc8ICQAIqoJfAwIBSAoLACHWQK5Y+"
        "J5Z/l//oAegBk9qpAAFF8DgABcyPQAydBBM/Rw8qGAAF72c52omhpr5jrhf/"
        "AIBIA4PABG7Nz7UTQ1wsfgD+"
        "7owwh10kglF8DcG3hIHew8l4ieNci1wsHnnDIUATPFhPLB8nQAqYI3iDACJRfA3Bt4Ns8FF8EI3ADqwKY0wcBwAAToQLkIG2OnF8DIcjLBiTPF"
        "snQhAlUQgHbPAWlFbIgwQEVQzDmMzUilF8FcG3hMgHHAJMxfwHfAtdJpvmBEVEAAYIcAAkjEB4AKAEPRqABztRNDTH9M/0//"
        "0BPQE0QE2cFmOlNs8IMcBnCDXSpPUMNCTMn8C4t4i5jAxEwT20wUhwQqOLCGRMeEhwAGXMdMH1AL7AOABwAmOFNQh+wTtQwLQ7R7tU1RiA/"
        "EGgvIA4PIt4HAiwRSUMNIPAd5tbSTBHoreJMEUjpElhAkj2zwzApUyxwDyo5Fb4t4kwAuOEzQC9ARQJIAQ9G4wECOECVnwAQHgJMAMiuAwFBUW"
        "FwCEMQLTAAHAAZPUAdCY0wUBqgLXGAHiINdJwg/"
        "ypiB41yLXCwfyaHBTEddJqTYCmNMHAcAAEqEB5DDIywYBzxbJ0FADACBZ9KhvpSCUAvQEMJIybeICACg0A4AQ9FqZECOECUBE8AEBkjAx4gBmM"
        "SLAFZwy9AQQI4QJUELwAQHgIsAWmDIChAn0czAB4DAyIMAfkzD0BODAIJJtAeDyLG0B");
    with_tvm_code(
        "wallet-v4-r2",
        "te6cckECFAEAAtQAART/APSkE/S88sgLAQIBIAIDAgFIBAUE+PKDCNcYINMf0x/THwL4I7vyZO1E0NMf0x/T//"
        "QE0VFDuvKhUVG68qIF+QFUEGT5EPKj+AAkpMjLH1JAyx9SMMv/"
        "UhD0AMntVPgPAdMHIcAAn2xRkyDXSpbTB9QC+wDoMOAhwAHjACHAAuMAAcADkTDjDQOkyMsfEssfy/"
        "8QERITAubQAdDTAyFxsJJfBOAi10nBIJJfBOAC0x8hghBwbHVnvSKCEGRzdHK9sJJfBeAD+kAwIPpEAcjKB8v/"
        "ydDtRNCBAUDXIfQEMFyBAQj0Cm+hMbOSXwfgBdM/"
        "yCWCEHBsdWe6kjgw4w0DghBkc3RyupJfBuMNBgcCASAICQB4AfoA9AQw+CdvIjBQCqEhvvLgUIIQcGx1Z4MesXCAGFAEywUmzxZY+"
        "gIZ9ADLaRfLH1Jgyz8gyYBA+wAGAIpQBIEBCPRZMO1E0IEBQNcgyAHPFvQAye1UAXKwjiOCEGRzdHKDHrFwgBhQBcsFUAPPFiP6AhPLassfyz/"
        "JgED7AJJfA+ICASAKCwBZvSQrb2omhAgKBrkPoCGEcNQICEekk30pkQzmkD6f+YN4EoAbeBAUiYcVnzGEAgFYDA0AEbjJftRNDXCx+"
        "AA9sp37UTQgQFA1yH0BDACyMoHy//J0AGBAQj0Cm+hMYAIBIA4PABmtznaiaEAga5Drhf/AABmvHfaiaEAQa5DrhY/AAG7SB/"
        "oA1NQi+QAFyMoHFcv/ydB3dIAYyMsFywIizxZQBfoCFMtrEszMyXP7AMhAFIEBCPRR8qcCAHCBAQjXGPoA0z/"
        "IVCBHgQEI9FHyp4IQbm90ZXB0gBjIywXLAlAGzxZQBPoCFMtqEssfyz/Jc/sAAgBsgQEI1xj6ANM/"
        "MFIkgQEI9Fnyp4IQZHN0cnB0gBjIywXLAlAFzxZQA/oCE8tqyx8Syz/Jc/sAAAr0AMntVGliJeU=");
    return map;
  }();
  return map;
}
}  // namespace

td::Result<td::Ref<vm::Cell>> SmartContractCode::load(td::Slice name) {
  auto& map = get_map();
  auto it = map.find(name);
  if (it == map.end()) {
    return td::Status::Error(PSLICE() << "Can't load td::Ref<vm::Cell> " << name);
  }
  return it->second;
}

td::Span<int> SmartContractCode::get_revisions(Type type) {
  switch (type) {
    case Type::WalletV3: {
      static int res[] = {1, 2};
      return res;
    }
    case Type::ManualDns: {
      static int res[] = {-1, 1};
      return res;
    }
    case Type::PaymentChannel: {
      static int res[] = {-1};
      return res;
    }
    case Type::WalletV4: {
      static int res[] = {2};
      return res;
    }
  }
  return {};
}

td::Result<int> SmartContractCode::validate_revision(Type type, int revision) {
  auto revisions = get_revisions(type);
  if (revisions.empty()) {
    return td::Status::Error("Unknown smart contract code type");
  }
  if (revision == -1) {
    if (revisions[0] == -1) {
      return -1;
    }
    return revisions[revisions.size() - 1];
  }
  if (revision == 0) {
    return revisions[revisions.size() - 1];
  }
  for (auto x : revisions) {
    if (x == revision) {
      return revision;
    }
  }
  return td::Status::Error("No such revision");
}

td::Ref<vm::Cell> SmartContractCode::get_code(Type type, int ext_revision) {
  auto r_revision = validate_revision(type, ext_revision);
  if (r_revision.is_error()) {
    return {};
  }
  auto revision = r_revision.move_as_ok();
  auto basename = [](Type type) -> td::Slice {
    switch (type) {
      case Type::WalletV3:
        return "wallet3";
      case Type::ManualDns:
        return "dns-manual";
      case Type::PaymentChannel:
        return "payment-channel";
      case Type::WalletV4:
        return "wallet-v4";
    }
    return {};
  }(type);
  if (basename.empty()) {
    return {};
  }
  if (revision == -1) {
    auto r_code = load(basename);
    if (r_code.is_error()) {
      return {};
    }
    return r_code.move_as_ok();
  }
  auto r_code = load(PSLICE() << basename << "-r" << revision);
  if (r_code.is_error()) {
    return {};
  }
  return r_code.move_as_ok();
}

}  // namespace ton

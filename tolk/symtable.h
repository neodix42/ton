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
*/
#pragma once

#include "src-file.h"
#include "fwd-declarations.h"
#include "crypto/common/refint.h"
#include <unordered_map>
#include <variant>
#include <vector>

namespace tolk {

struct Symbol {
  std::string name;
  SrcLocation loc;

  Symbol(std::string name, SrcLocation loc)
    : name(std::move(name))
    , loc(loc) {
  }

  virtual ~Symbol() = default;

  template<class ConstTPtr>
  ConstTPtr try_as() const {
#ifdef TOLK_DEBUG
    assert(this != nullptr);
#endif
    return dynamic_cast<ConstTPtr>(this);
  }

  void check_import_exists_when_used_from(FunctionPtr cur_f, SrcLocation used_loc) const;
};

struct LocalVarData final : Symbol {
  enum {
    flagMutateParameter = 1,    // parameter was declared with `mutate` keyword
    flagImmutable = 2,          // variable was declared via `val` (not `var`)
    flagLateInit = 4,           // variable was declared via `lateinit` (not assigned at declaration)
    flagUsedAsLVal = 8,         // variable is assigned or in another way used as lvalue inside a function
  };

  AnyTypeV type_node;               // either at declaration `var x:int`, or if omitted, from assigned value `var x=2`
  TypePtr declared_type = nullptr;  // = resolved type_node
  AnyExprV default_value = nullptr; // for function parameters, if it has a default value
  int flags;
  int param_idx;                    // 0...N for function parameters, -1 for local vars
  std::vector<int> ir_idx;

  LocalVarData(std::string name, SrcLocation loc, AnyTypeV type_node, AnyExprV default_value, int flags, int param_idx)
    : Symbol(std::move(name), loc)
    , type_node(type_node)
    , default_value(default_value)
    , flags(flags)
    , param_idx(param_idx) {
  }
  LocalVarData(std::string name, SrcLocation loc, TypePtr declared_type, AnyExprV default_value, int flags, int param_idx)
    : Symbol(std::move(name), loc)
    , type_node(nullptr)         // for built-in functions (their parameters)
    , declared_type(declared_type)
    , default_value(default_value)
    , flags(flags)
    , param_idx(param_idx) {
  }

  bool is_parameter() const { return param_idx >= 0; }

  bool is_immutable() const { return flags & flagImmutable; }
  bool is_lateinit() const { return flags & flagLateInit; }
  bool is_mutate_parameter() const { return flags & flagMutateParameter; }
  bool is_used_as_lval() const { return flags & flagUsedAsLVal; }
  bool has_default_value() const { return default_value != nullptr; }

  LocalVarData* mutate() const { return const_cast<LocalVarData*>(this); }
  void assign_used_as_lval();
  void assign_ir_idx(std::vector<int>&& ir_idx);
  void assign_resolved_type(TypePtr declared_type);
  void assign_inferred_type(TypePtr inferred_type);
  void assign_default_value(AnyExprV default_value);
};

struct FunctionBodyCode;
struct FunctionBodyAsm;
struct FunctionBodyBuiltin;
struct GenericsDeclaration;

typedef std::variant<
  FunctionBodyCode*,
  FunctionBodyAsm*,
  FunctionBodyBuiltin*
> FunctionBody;

struct FunctionData final : Symbol {
  static constexpr int EMPTY_TVM_METHOD_ID = -10;

  enum {
    flagTypeInferringDone = 4,  // type inferring step of function's body (all AST nodes assigning v->inferred_type) is done
    flagUsedAsNonCall = 8,      // used not only as `f()`, but as a 1-st class function (assigned to var, pushed to tuple, etc.)
    flagMarkedAsPure = 16,      // declared as `pure`, can't call impure and access globals, unused invocations are optimized out
    flagImplicitReturn = 32,    // control flow reaches end of function, so it needs implicit return at the end
    flagContractGetter = 64,    // was declared via `get func(): T`, tvm_method_id is auto-assigned
    flagIsEntrypoint = 128,    // it's `main` / `onExternalMessage` / etc.
    flagHasMutateParams = 256,  // has parameters declared as `mutate`
    flagAcceptsSelf = 512,      // is a member function (has `self` first parameter)
    flagReturnsSelf = 1024,     // return type is `self` (returns the mutated 1st argument), calls can be chainable
    flagReallyUsed = 2048,      // calculated via dfs from used functions; declared but unused functions are not codegenerated
    flagCompileTimeVal = 4096,  // calculated only at compile-time for constant arguments: `ton("0.05")`, `stringCrc32`, and others
    flagCompileTimeGen = 8192,  // at compile-time it's handled specially, not as a regular function: `T.toCell`, etc.
    flagAllowAnyWidthT = 16384, // for built-in generic functions that <T> is not restricted to be 1-slot type
    flagManualOnBounce = 32768, // for onInternalMessage, don't insert "if (isBounced) return"
  };

  int tvm_method_id = EMPTY_TVM_METHOD_ID;
  int flags;
  FunctionInlineMode inline_mode;
  int n_times_called = 0;                     // calculated while building call graph; 9999 for recursions

  std::string method_name;                    // for `fun Container<T>.store<U>` here is "store"
  AnyTypeV receiver_type_node;                // for `fun Container<T>.store<U>` here is `Container<T>`
  TypePtr receiver_type = nullptr;            // = resolved receiver_type_node

  std::vector<LocalVarData> parameters;
  std::vector<int> arg_order, ret_order;
  AnyTypeV return_type_node;                  // may be nullptr, meaning "auto infer"
  TypePtr declared_return_type = nullptr;     // = resolved return_type_node
  TypePtr inferred_return_type = nullptr;     // assigned on type inferring
  TypePtr inferred_full_type = nullptr;       // assigned on type inferring, it's TypeDataFunCallable(params -> return)

  const GenericsDeclaration* genericTs;
  const GenericsSubstitutions* substitutedTs;
  FunctionPtr base_fun_ref = nullptr;             // for `f<int>`, here is `f<T>`
  FunctionBody body;
  AnyV ast_root;                                  // V<ast_function_declaration> for user-defined (not builtin)

  FunctionData(std::string name, SrcLocation loc, std::string method_name, AnyTypeV receiver_type_node, AnyTypeV return_type_node, std::vector<LocalVarData> parameters, int initial_flags, FunctionInlineMode inline_mode, const GenericsDeclaration* genericTs, const GenericsSubstitutions* substitutedTs, FunctionBody body, AnyV ast_root)
    : Symbol(std::move(name), loc)
    , flags(initial_flags)
    , inline_mode(inline_mode)
    , method_name(std::move(method_name))
    , receiver_type_node(receiver_type_node)
    , parameters(std::move(parameters))
    , return_type_node(return_type_node)
    , genericTs(genericTs)
    , substitutedTs(substitutedTs)
    , body(body)
    , ast_root(ast_root) {
  }
  FunctionData(std::string name, SrcLocation loc, std::string method_name, TypePtr receiver_type, TypePtr declared_return_type, std::vector<LocalVarData> parameters, int initial_flags, FunctionInlineMode inline_mode, const GenericsDeclaration* genericTs, const GenericsSubstitutions* substitutedTs, FunctionBody body, AnyV ast_root)
    : Symbol(std::move(name), loc)
    , flags(initial_flags)
    , inline_mode(inline_mode)
    , method_name(std::move(method_name))
    , receiver_type_node(nullptr)
    , receiver_type(receiver_type)
    , parameters(std::move(parameters))
    , return_type_node(nullptr)            // for built-in functions, defined in sources
    , declared_return_type(declared_return_type)
    , genericTs(genericTs)
    , substitutedTs(substitutedTs)
    , body(body)
    , ast_root(ast_root) {
  }

  std::string as_human_readable() const;

  const std::vector<int>* get_arg_order() const {
    return arg_order.empty() ? nullptr : &arg_order;
  }
  const std::vector<int>* get_ret_order() const {
    return ret_order.empty() ? nullptr : &ret_order;
  }

  int get_num_params() const { return static_cast<int>(parameters.size()); }
  const LocalVarData& get_param(int idx) const { return parameters[idx]; }
  LocalVarPtr find_param(std::string_view name) const;

  bool is_code_function() const { return std::holds_alternative<FunctionBodyCode*>(body); }
  bool is_asm_function() const { return std::holds_alternative<FunctionBodyAsm*>(body); }
  bool is_builtin_function() const { return ast_root == nullptr; }
  bool is_method() const { return !method_name.empty(); }
  bool is_static_method() const { return is_method() && !does_accept_self(); }

  bool is_generic_function() const { return genericTs != nullptr; }
  bool is_instantiation_of_generic_function() const { return substitutedTs != nullptr; }

  bool is_inlined_in_place() const { return inline_mode == FunctionInlineMode::inlineInPlace; }
  bool is_type_inferring_done() const { return flags & flagTypeInferringDone; }
  bool is_used_as_noncall() const { return flags & flagUsedAsNonCall; }
  bool is_marked_as_pure() const { return flags & flagMarkedAsPure; }
  bool is_implicit_return() const { return flags & flagImplicitReturn; }
  bool is_contract_getter() const { return flags & flagContractGetter; }
  bool has_tvm_method_id() const { return tvm_method_id != EMPTY_TVM_METHOD_ID; }
  bool is_entrypoint() const { return flags & flagIsEntrypoint; }
  bool has_mutate_params() const { return flags & flagHasMutateParams; }
  bool does_accept_self() const { return flags & flagAcceptsSelf; }
  bool does_return_self() const { return flags & flagReturnsSelf; }
  bool does_mutate_self() const { return (flags & flagAcceptsSelf) && parameters[0].is_mutate_parameter(); }
  bool is_really_used() const { return flags & flagReallyUsed; }
  bool is_compile_time_const_val() const { return flags & flagCompileTimeVal; }
  bool is_compile_time_special_gen() const { return flags & flagCompileTimeGen; }
  bool is_variadic_width_T_allowed() const { return flags & flagAllowAnyWidthT; }
  bool is_manual_on_bounce() const { return flags & flagManualOnBounce; }

  bool does_need_codegen() const;

  FunctionData* mutate() const { return const_cast<FunctionData*>(this); }
  void assign_resolved_receiver_type(TypePtr receiver_type, std::string&& name_prefix);
  void assign_resolved_genericTs(const GenericsDeclaration* genericTs);
  void assign_resolved_type(TypePtr declared_return_type);
  void assign_inferred_type(TypePtr inferred_return_type, TypePtr inferred_full_type);
  void assign_is_used_as_noncall();
  void assign_is_implicit_return();
  void assign_is_type_inferring_done();
  void assign_is_really_used();
  void assign_inline_mode_in_place();
  void assign_arg_order(std::vector<int>&& arg_order);
};

struct GlobalVarData final : Symbol {
  enum {
    flagReallyUsed = 1,          // calculated via dfs from used functions; unused globals are not codegenerated
  };

  AnyTypeV type_node;                 // `global a: int;` always exists, declaring globals without type is prohibited
  TypePtr declared_type = nullptr;    // = resolved type_node
  int flags = 0;

  GlobalVarData(std::string name, SrcLocation loc, AnyTypeV type_node)
    : Symbol(std::move(name), loc)
    , type_node(type_node) {
  }

  bool is_really_used() const { return flags & flagReallyUsed; }

  GlobalVarData* mutate() const { return const_cast<GlobalVarData*>(this); }
  void assign_resolved_type(TypePtr declared_type);
  void assign_is_really_used();
};

struct GlobalConstData final : Symbol {
  AnyTypeV type_node;                 // exists for `const op: int = rhs`, otherwise nullptr
  TypePtr declared_type = nullptr;    // = resolved type_node
  TypePtr inferred_type = nullptr;
  AnyExprV init_value;

  GlobalConstData(std::string name, SrcLocation loc, AnyTypeV type_node, AnyExprV init_value)
    : Symbol(std::move(name), loc)
    , type_node(type_node)
    , init_value(init_value) {
  }

  GlobalConstData* mutate() const { return const_cast<GlobalConstData*>(this); }
  void assign_resolved_type(TypePtr declared_type);
  void assign_inferred_type(TypePtr inferred_type);
  void assign_init_value(AnyExprV init_value);
};

struct AliasDefData final : Symbol {
  AnyTypeV underlying_type_node;
  TypePtr underlying_type = nullptr;    // = resolved underlying_type_node

  const GenericsDeclaration* genericTs;
  const GenericsSubstitutions* substitutedTs;
  AliasDefPtr base_alias_ref = nullptr;           // for `Response<int>`, here is `Response<T>`
  AnyV ast_root;                                  // V<ast_type_alias_declaration>

  AliasDefData(std::string name, SrcLocation loc, AnyTypeV underlying_type_node, const GenericsDeclaration* genericTs, const GenericsSubstitutions* substitutedTs, AnyV ast_root)
    : Symbol(std::move(name), loc)
    , underlying_type_node(underlying_type_node)
    , genericTs(genericTs)
    , substitutedTs(substitutedTs)
    , ast_root(ast_root) {
  }

  std::string as_human_readable() const;

  bool is_generic_alias() const { return genericTs != nullptr; }
  bool is_instantiation_of_generic_alias() const { return substitutedTs != nullptr; }

  AliasDefData* mutate() const { return const_cast<AliasDefData*>(this); }
  void assign_resolved_genericTs(const GenericsDeclaration* genericTs);
  void assign_resolved_type(TypePtr underlying_type);
};

struct StructFieldData final : Symbol {
  int field_idx;
  AnyTypeV type_node;
  TypePtr declared_type = nullptr;      // = resolved type_node
  AnyExprV default_value;               // nullptr if no default

  bool has_default_value() const { return default_value != nullptr; }

  StructFieldData* mutate() const { return const_cast<StructFieldData*>(this); }
  void assign_resolved_type(TypePtr declared_type);
  void assign_default_value(AnyExprV default_value);

  StructFieldData(std::string name, SrcLocation loc, int field_idx, AnyTypeV type_node, AnyExprV default_value)
    : Symbol(std::move(name), loc)
    , field_idx(field_idx)
    , type_node(type_node)
    , default_value(default_value) {
  }
};

struct StructData final : Symbol {
  enum class Overflow1023Policy {     // annotation @overflow1023_policy above a struct
    not_specified,
    suppress,
  };

  struct PackOpcode {
    int64_t pack_prefix;
    int prefix_len;

    PackOpcode(int64_t pack_prefix, int prefix_len)
      : pack_prefix(pack_prefix), prefix_len(prefix_len) {}

    bool exists() const { return prefix_len != 0; }

    std::string format_as_slice() const;    // "x{...}" (or "b{...}")
  };

  std::vector<StructFieldPtr> fields;
  PackOpcode opcode;
  Overflow1023Policy overflow1023_policy;

  const GenericsDeclaration* genericTs;
  const GenericsSubstitutions* substitutedTs;
  StructPtr base_struct_ref = nullptr;            // for `Container<int>`, here is `Container<T>`
  AnyV ast_root;                                  // V<ast_struct_declaration>

  int get_num_fields() const { return static_cast<int>(fields.size()); }
  StructFieldPtr get_field(int i) const { return fields.at(i); }
  StructFieldPtr find_field(std::string_view field_name) const;

  bool is_generic_struct() const { return genericTs != nullptr; }
  bool is_instantiation_of_generic_struct() const { return substitutedTs != nullptr; }

  StructData* mutate() const { return const_cast<StructData*>(this); }
  void assign_resolved_genericTs(const GenericsDeclaration* genericTs);

  StructData(std::string name, SrcLocation loc, std::vector<StructFieldPtr>&& fields, PackOpcode opcode, Overflow1023Policy overflow1023_policy, const GenericsDeclaration* genericTs, const GenericsSubstitutions* substitutedTs, AnyV ast_root)
    : Symbol(std::move(name), loc)
    , fields(std::move(fields))
    , opcode(opcode)
    , overflow1023_policy(overflow1023_policy)
    , genericTs(genericTs)
    , substitutedTs(substitutedTs)
    , ast_root(ast_root) {
  }

  std::string as_human_readable() const;
};

struct TypeReferenceUsedAsSymbol final : Symbol {
  TypePtr resolved_type;

  TypeReferenceUsedAsSymbol(std::string name, SrcLocation loc, TypePtr resolved_type)
    : Symbol(std::move(name), loc)
    , resolved_type(resolved_type) {
  }
};

class GlobalSymbolTable {
  std::unordered_map<uint64_t, const Symbol*> entries;

  static uint64_t key_hash(std::string_view name_key) {
    return std::hash<std::string_view>{}(name_key);
  }

public:
  void add_function(FunctionPtr f_sym);
  void add_global_var(GlobalVarPtr g_sym);
  void add_global_const(GlobalConstPtr c_sym);
  void add_type_alias(AliasDefPtr a_sym);
  void add_struct(StructPtr s_sym);

  void replace_function(FunctionPtr f_sym);

  const Symbol* lookup(std::string_view name) const {
    const auto it = entries.find(key_hash(name));
    return it == entries.end() ? nullptr : it->second;
  }
};

const Symbol* lookup_global_symbol(std::string_view name);
FunctionPtr lookup_function(std::string_view name);
std::vector<FunctionPtr> lookup_methods_with_name(std::string_view name);

}  // namespace tolk

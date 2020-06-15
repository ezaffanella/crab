#pragma once

#include <crab/types/types.hpp>
#include <crab/domains/abstract_domain_operators.hpp>
#include <crab/iterators/thresholds.hpp>
#include <vector>

namespace crab {

namespace domains {

template <class Dom> struct abstract_domain_traits;

/**
 * All abstract domains must derive from the abstract_domain class
 * and expose publicly all its public typedef's.
 *
 * This is a sample of how to implement a new abstract domain:
 *
 * template<typename Number, typename VariableName>
 * class my_new_domain final: public
 *abstract_domain<my_new_domain<Number,VariableName>> {
 *   ...
 *   bool is_bottom() {...}
 *   bool is_top() {...}
 *   ...
 * };
 *
 *
 * template<typename Number, typename VariableName>
 * struct abstract_domain_traits<my_new_domain<Number,VariableName>> {
 *   typedef Number number_t;
 *   typedef VariableName varname_t;
 * };
 **/
template <class Dom> class abstract_domain {
public:
  typedef typename abstract_domain_traits<Dom>::number_t number_t;
  typedef typename abstract_domain_traits<Dom>::varname_t varname_t;

  typedef ikos::linear_expression<number_t, varname_t> linear_expression_t;
  typedef ikos::linear_constraint<number_t, varname_t> linear_constraint_t;
  typedef ikos::linear_constraint_system<number_t, varname_t>
      linear_constraint_system_t;
  typedef ikos::disjunctive_linear_constraint_system<number_t, varname_t>
      disjunctive_linear_constraint_system_t;
  typedef ikos::variable<number_t, varname_t> variable_t;
  typedef std::vector<variable_t> variable_vector_t;
  typedef reference_constraint<number_t,varname_t> reference_constraint_t;
  
  abstract_domain() {}

  virtual ~abstract_domain(){};

  static Dom top() {
    Dom abs;
    abs.set_to_top();
    return abs;
  }

  static Dom bottom() {
    Dom abs;
    abs.set_to_bottom();
    return abs;
  }

  /**************************** Lattice operations ****************************/

  // set *this to top
  virtual void set_to_top() = 0;
  // set *this to bottom
  virtual void set_to_bottom() = 0;
  // return true if the abstract state is bottom
  virtual bool is_bottom() = 0;
  // return true if the abstract state is top
  virtual bool is_top() = 0;

  // Inclusion operator: return true if *this is equal or more precise than abs
  virtual bool operator<=(Dom abs) = 0;
  // Join operator: join(*this, abs)
  virtual Dom operator|(Dom abs) = 0;
  // *this = join(*this, abs)
  virtual void operator|=(Dom abs) = 0;
  // Meet operator: meet(*this, abs)
  virtual Dom operator&(Dom abs) = 0;
  // Widening operator: widening(*this, abs)
  virtual Dom operator||(Dom abs) = 0;
  // Narrowing operator: narrowing(*this, abs)
  virtual Dom operator&&(Dom abs) = 0;
  // Widening with thresholds: widening_ts(*this, abs)
  virtual Dom
  widening_thresholds(Dom abs,
                      const crab::iterators::thresholds<number_t> &ts) = 0;

  /**************************** Arithmetic operations *************************/
  // x := y op z
  virtual void apply(operation_t op, variable_t x, variable_t y,
                     variable_t z) = 0;
  // x := y op k
  virtual void apply(operation_t op, variable_t x, variable_t y,
                     number_t k) = 0;
  // x := e
  virtual void assign(variable_t x, linear_expression_t e) = 0;
  // add all constraints \in csts 
  virtual void operator+=(linear_constraint_system_t csts) = 0;
  // x := y op z
  virtual void apply(bitwise_operation_t op, variable_t x, variable_t y,
                     variable_t z) = 0;
  // x := y op k
  virtual void apply(bitwise_operation_t op, variable_t x, variable_t y,
                     number_t k) = 0;
  // dst := src
  virtual void apply(int_conv_operation_t op, variable_t dst,
                     variable_t src) = 0;

  /**************************** Boolean operations ****************************/
  // lhs := rhs
  virtual void assign_bool_cst(variable_t lhs, linear_constraint_t rhs) = 0;
  // lhs := not(rhs) if is_not_rhs
  // lhs := rhs      otherwise
  virtual void assign_bool_var(variable_t lhs, variable_t rhs,
                               bool is_not_rhs) = 0;
  // x := y op z
  virtual void apply_binary_bool(bool_operation_t op, variable_t x,
                                 variable_t y, variable_t z) = 0;
  // assume(not(v)) if is_negated
  // assume(v)      otherwise
  virtual void assume_bool(variable_t v, bool is_negated) = 0;

  /**************************** Array operations *****************************/
  // make a fresh array with contents a[j] initialized to val such that
  // j \in [lb_idx,ub_idx] and j % elem_size == val.
  // elem_size is in bytes.
  virtual void array_init(variable_t a, linear_expression_t elem_size,
                          linear_expression_t lb_idx,
                          linear_expression_t ub_idx,
                          linear_expression_t val) = 0;
  // lhs := a[i] where elem_size is in bytes
  virtual void array_load(variable_t lhs, variable_t a,
                          linear_expression_t elem_size,
                          linear_expression_t i) = 0;
  // a[i] := val where elem_size is in bytes
  virtual void array_store(variable_t a, linear_expression_t elem_size,
                           linear_expression_t i, linear_expression_t val,
                           bool is_strong_update) = 0;
  // a_new = a_old[i <- val] where elem_size is in bytes
  virtual void array_store(variable_t a_new, variable_t a_old,
                           linear_expression_t elem_size, linear_expression_t i,
                           linear_expression_t val, bool is_strong_update) = 0;
  // forall i<=k<j and k % elem_size == 0 :: a[k] := val.
  // elem_size is in bytes
  virtual void array_store_range(variable_t a, linear_expression_t elem_size,
                                 linear_expression_t i, linear_expression_t j,
                                 linear_expression_t val) = 0;
  // forall i<=k<j and k % elem_size == 0 :: a_new = a_old[k <- val].
  // elem_size is in bytes
  virtual void array_store_range(variable_t a_new, variable_t a_old,
                                 linear_expression_t elem_size,
                                 linear_expression_t i, linear_expression_t j,
                                 linear_expression_t val) = 0;
  // forall i :: a[i] := b[i]
  virtual void array_assign(variable_t a, variable_t b) = 0;
  /**************************** Reference operations ************************/
  // A reference is a non-deterministic address within a region
  // 
  // There are two operations that can create references to a region:
  // - ref_make
  // - ref_gep
  //
  // The rest of operations (except ref_assume) take a reference to a
  // region and read/write from/to it.
  //
  // Initialize region. If reg already exists then error.
  virtual void region_init(memory_region reg) = 0;
  // Create a new reference ref to region reg.
  virtual void ref_make(variable_t ref, memory_region reg) = 0;
  // Read the content of reference ref within reg. The content is
  // stored in res.
  virtual void ref_load(variable_t ref, memory_region reg, variable_t res) = 0;
  // Write the content of val to the address pointed by ref in region
  // reg.
  virtual void ref_store(variable_t ref, memory_region reg, linear_expression_t val) = 0;
  // Create a new reference ref2 to region reg2.
  // The reference ref2 is created by adding offset to ref1.
  virtual void ref_gep(variable_t ref1, memory_region reg1,
		       variable_t ref2, memory_region reg2,
		       linear_expression_t offset) = 0;
  // Treat memory pointed by ref  as an array and perform an array load.
  virtual void ref_load_from_array(variable_t lhs, variable_t ref, memory_region region,
				   linear_expression_t index, linear_expression_t elem_size) = 0;
  // Treat region as an array and perform an array store.
  virtual void ref_store_to_array(variable_t ref, memory_region region,
				  linear_expression_t index, linear_expression_t elem_size,
				  linear_expression_t val) = 0;
  // Add constraints between references 
  virtual void ref_assume(reference_constraint_t cst) = 0;
  /**************************** Backward arithmetic operations ***************/
  // x = y op z
  // Substitute x with y op z in the abstract value
  // The result is meet with invariant.
  virtual void backward_apply(operation_t op, variable_t x, variable_t y,
                              variable_t z, Dom invariant) = 0;
  // x = y op k
  // Substitute x with y op k in the abstract value
  // The result is meet with invariant.
  virtual void backward_apply(operation_t op, variable_t x, variable_t y,
                              number_t k, Dom invariant) = 0;
  // x = e
  // Substitute x with e in the abstract value
  // The result is meet with invariant.
  virtual void backward_assign(variable_t x, linear_expression_t e,
                               Dom invariant) = 0;

  /**************************** Backward boolean operations ******************/
  virtual void backward_assign_bool_cst(variable_t lhs, linear_constraint_t rhs,
                                        Dom invariant) = 0;
  virtual void backward_assign_bool_var(variable_t lhs, variable_t rhs,
                                        bool is_not_rhs, Dom invariant) = 0;
  virtual void backward_apply_binary_bool(bool_operation_t op, variable_t x,
                                          variable_t y, variable_t z,
                                          Dom invariant) = 0;

  /**************************** Backward array operations ******************/
  virtual void backward_array_init(variable_t a, linear_expression_t elem_size,
                                   linear_expression_t lb_idx,
                                   linear_expression_t ub_idx,
                                   linear_expression_t val, Dom invariant) = 0;
  virtual void backward_array_load(variable_t lhs, variable_t a,
                                   linear_expression_t elem_size,
                                   linear_expression_t i, Dom invariant) = 0;
  virtual void backward_array_store(variable_t a, linear_expression_t elem_size,
                                    linear_expression_t i,
                                    linear_expression_t v,
                                    bool is_strong_update, Dom invariant) = 0;
  virtual void backward_array_store(variable_t a_new, variable_t a_old,
                                    linear_expression_t elem_size,
                                    linear_expression_t i,
                                    linear_expression_t v,
                                    bool is_strong_update, Dom invariant) = 0;
  virtual void
  backward_array_store_range(variable_t a, linear_expression_t elem_size,
                             linear_expression_t i, linear_expression_t j,
                             linear_expression_t v, Dom invariant) = 0;
  virtual void backward_array_store_range(variable_t a_new, variable_t a_old,
                                          linear_expression_t elem_size,
                                          linear_expression_t i,
                                          linear_expression_t j,
                                          linear_expression_t v,
                                          Dom invariant) = 0;
  virtual void backward_array_assign(variable_t a, variable_t b,
                                     Dom invariant) = 0;

  /**************************** Miscellaneous operations ****************/
  // forget v
  virtual void operator-=(variable_t v) = 0;

  // Convert the abstract state into a conjunction of linear constraints.
  virtual linear_constraint_system_t to_linear_constraint_system() = 0;

  // Convert the abstract state into a disjunction of conjunction
  // of linear constraints.
  virtual disjunctive_linear_constraint_system_t
  to_disjunctive_linear_constraint_system() = 0;

  // Rename in the abstract state the variables "from" with those from to.
  // 
  // If any variable from "to" exists already in the abstract state
  // then an error will be raised. This might be a bit restrictive and
  // it can be relaxed if needed in the future. 
  virtual void rename(const variable_vector_t &from,
                      const variable_vector_t &to) = 0;

  // Normalize the abstract domain if such notion exists.
  virtual void normalize() = 0;

  // Reduce the size of the abstract domain representation.
  virtual void minimize() = 0;

  // Forget variables form the abstract domain
  virtual void forget(const variable_vector_t &variables) = 0;

  // Project the abstract domain onto variables (dual to forget)
  virtual void project(const variable_vector_t &variables) = 0;

  // Make a new copy of var without relating var with new_var
  virtual void expand(variable_t var, variable_t new_var) = 0;

  // Function whose semantics is defined by the particular abstract
  // domain
  virtual void intrinsic(std::string name,
			 const variable_vector_t &inputs,
			 const variable_vector_t &outputs) = 0;

  virtual void backward_intrinsic(std::string name,
				  const variable_vector_t &inputs,
				  const variable_vector_t &outputs,
				  Dom invariant) = 0;

  // Print the internal state of the abstract domain
  virtual void write(crab::crab_os &o) = 0;
  
  friend crab::crab_os &operator<<(crab::crab_os &o, abstract_domain<Dom> &dom) {
    dom.write(o);
    return o;
  }
};

} // end namespace domains
} // end namespace crab


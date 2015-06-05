/*******************************************************************************
 * Generic implementation of non-relational domains.
 ******************************************************************************/

#ifndef IKOS_SEPARATE_DOMAINS_HPP
#define IKOS_SEPARATE_DOMAINS_HPP

#include <iostream>
#include <ikos/common/types.hpp>
#include <ikos/algorithms/patricia_trees.hpp>

namespace ikos {
  
  template < typename Key, typename Value >
  class separate_domain: public writeable {
    
  private:
    typedef patricia_tree< Key, Value > patricia_tree_t;
    typedef typename patricia_tree_t::unary_op_t unary_op_t;
    typedef typename patricia_tree_t::binary_op_t binary_op_t;
    typedef typename patricia_tree_t::partial_order_t partial_order_t;

  public:
    typedef separate_domain< Key, Value > separate_domain_t;
    typedef typename patricia_tree_t::iterator iterator;
    
  private:
    bool _is_bottom;
    patricia_tree_t _tree;
    
  private:
    class bottom_found { };
    
    class join_op: public binary_op_t {
      
      boost::optional< Value > apply(Value x, Value y) {
	Value z = x.operator|(y);
	if (z.is_top()) {
	  return boost::optional< Value >();
	} else {
	  return boost::optional< Value >(z);
	}
      };
      
      bool default_is_absorbing() {
	return true;
      }

    }; // class join_op

    class widening_op: public binary_op_t {
      
      boost::optional< Value > apply(Value x, Value y) {
	Value z = x.operator||(y);
	if (z.is_top()) {
	  return boost::optional< Value >();
	} else {
	  return boost::optional< Value >(z);
	}
      };
      
      bool default_is_absorbing() {
	return true;
      }

    }; // class widening_op

    class meet_op: public binary_op_t {
      
      boost::optional< Value > apply(Value x, Value y) {
	Value z = x.operator&(y);
	if (z.is_bottom()) {
	  throw bottom_found();
	} else {
	  return boost::optional< Value >(z);
	}
      };
      
      bool default_is_absorbing() {
	return false;
      }
      
    }; // class meet_op
    
    class narrowing_op: public binary_op_t {
      
      boost::optional< Value > apply(Value x, Value y) {
	Value z = x.operator&&(y);
	if (z.is_bottom()) {
	  throw bottom_found();
	} else {
	  return boost::optional< Value >(z);
	}
      };
      
      bool default_is_absorbing() {
	return false;
      }
      
    }; // class narrowing_op

    class domain_po: public partial_order_t {

      bool leq(Value x, Value y) {
	return x.operator<=(y);
      }
      
      bool default_is_top() {
	return true;
      }
      
    }; // class domain_po

  public:
    static separate_domain_t top() {
      return separate_domain_t();
    }
    
    static separate_domain_t bottom() {
      return separate_domain_t(false);
    }
    
  private:
    static patricia_tree_t apply_operation(binary_op_t& o, patricia_tree_t t1, patricia_tree_t t2) {
      t1.merge_with(t2, o);
      return t1;
    }

  private:
    separate_domain(patricia_tree_t t): _is_bottom(false), _tree(t) { }
    
    separate_domain(bool b): _is_bottom(!b) { }
    
  public:
    separate_domain(): _is_bottom(false) { }

    separate_domain(const separate_domain_t& e): writeable(), _is_bottom(e._is_bottom), _tree(e._tree) { }

    separate_domain_t& operator=(separate_domain_t e) {
      this->_is_bottom = e._is_bottom;
      this->_tree = e._tree;
      return *this;
    }

    iterator begin() {
      if (this->is_bottom()) {
	throw error("Separate domain: trying to invoke iterator on bottom");
      } else {
	return this->_tree.begin();
      }
    }
    
    iterator end() {
      if (this->is_bottom()) {
	throw error("Separate domain: trying to invoke iterator on bottom");
      } else {
	return this->_tree.end();
      }
    }
    
    bool is_bottom() {
      return this->_is_bottom;
    }

    bool is_top() {
      return (!this->is_bottom() && this->_tree.size() == 0);
    }
    
    bool operator<=(separate_domain_t e) {
      if (this->is_bottom()) {
	return true;
      } else if (e.is_bottom()) {
	return false;
      } else {
	domain_po po;
	return this->_tree.leq(e._tree, po);
      }
    }

    bool operator==(separate_domain_t e) {
      return (this->operator<=(e) && e.operator<=(*this));
    }
    
    // Join
    separate_domain_t operator|(separate_domain_t e) {
      if (this->is_bottom()) {
	return e;
      } else if(e.is_bottom()) {
	return *this;
      } else {
	join_op o;
	return separate_domain_t(apply_operation(o, this->_tree, e._tree));
      }
    }

    // Meet
    separate_domain_t operator&(separate_domain_t e) {
      if (this->is_bottom() || e.is_bottom()) {
	return this->bottom();
      } else {
	try {
	  meet_op o;
	  return separate_domain_t(apply_operation(o, this->_tree, e._tree));
	}
	catch (bottom_found& exc) {
	  return this->bottom();
	}
      }
    }

    // Widening
    separate_domain_t operator||(separate_domain_t e) {
      if (this->is_bottom()) {
	return e;
      } else if(e.is_bottom()) {
	return *this;
      } else {
	widening_op o;
	return separate_domain_t(apply_operation(o, this->_tree, e._tree));
      }
    }

    // Narrowing
    separate_domain_t operator&&(separate_domain_t e) {
      if (this->is_bottom() || e.is_bottom()) {
	return separate_domain_t(false);
      } else {
	try {
	  narrowing_op o;
	  return separate_domain_t(apply_operation(o, this->_tree, e._tree));
	}
	catch (bottom_found& exc) {
	  return this->bottom();
	}	
      }
    }
    
    void set(Key k, Value v) {
      if (!this->is_bottom()) {
	if (v.is_bottom()) {
	  this->_is_bottom = true;
	  this->_tree = patricia_tree_t();
	} else if (v.is_top()) {
	  this->_tree.remove(k);
	} else {
	  this->_tree.insert(k, v);
	}
      }
    }

    void set_to_bottom() {
      this->_is_bottom = true;
      this->_tree = patricia_tree_t();
    }

    separate_domain_t& operator-=(Key k) {
      if (!this->is_bottom()) {
	this->_tree.remove(k);
      }
      return *this;
    }
    
    Value operator[](Key k) {
      if (this->is_bottom()) {
	return Value::bottom();
      } else {
	boost::optional< Value > v = this->_tree.lookup(k);
	if (v) {
	  return *v;
	} else {
	  return Value::top();
	}
      }
    }

    std::ostream& write(std::ostream& o) {
      if (this->is_bottom()) {
	o << "_|_";
      } else {
	o << "{";
	for (typename patricia_tree_t::iterator it = this->_tree.begin(); it != this->_tree.end(); ) {
	  it->first.write(o);
	  o << " -> ";
	  it->second.write(o);
	  ++it;
	  if (it != this->_tree.end()) {
	    o << "; ";
	  }
	}
	o << "}";
      }
      return o;
    }
    
  }; // class separate_domain
  
} // namespace ikos

#endif // IKOS_SEPARATE_DOMAINS_HPP
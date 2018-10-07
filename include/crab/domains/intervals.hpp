/*******************************************************************************
 *
 * Standard domain of intervals.
 *
 * Author: Arnaud J. Venet (arnaud.j.venet@nasa.gov)
 *
 * Contributors: Alexandre C. D. Wimmers (alexandre.c.wimmers@nasa.gov)
 *
 * Notices:
 *
 * Copyright (c) 2011 United States Government as represented by the
 * Administrator of the National Aeronautics and Space Administration.
 * All Rights Reserved.
 *
 * Disclaimers:
 *
 * No Warranty: THE SUBJECT SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY OF
 * ANY KIND, EITHER EXPRESSED, IMPLIED, OR STATUTORY, INCLUDING, BUT NOT LIMITED
 * TO, ANY WARRANTY THAT THE SUBJECT SOFTWARE WILL CONFORM TO SPECIFICATIONS,
 * ANY IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE,
 * OR FREEDOM FROM INFRINGEMENT, ANY WARRANTY THAT THE SUBJECT SOFTWARE WILL BE
 * ERROR FREE, OR ANY WARRANTY THAT DOCUMENTATION, IF PROVIDED, WILL CONFORM TO
 * THE SUBJECT SOFTWARE. THIS AGREEMENT DOES NOT, IN ANY MANNER, CONSTITUTE AN
 * ENDORSEMENT BY GOVERNMENT AGENCY OR ANY PRIOR RECIPIENT OF ANY RESULTS,
 * RESULTING DESIGNS, HARDWARE, SOFTWARE PRODUCTS OR ANY OTHER APPLICATIONS
 * RESULTING FROM USE OF THE SUBJECT SOFTWARE.  FURTHER, GOVERNMENT AGENCY
 * DISCLAIMS ALL WARRANTIES AND LIABILITIES REGARDING THIRD-PARTY SOFTWARE,
 * IF PRESENT IN THE ORIGINAL SOFTWARE, AND DISTRIBUTES IT "AS IS."
 *
 * Waiver and Indemnity:  RECIPIENT AGREES TO WAIVE ANY AND ALL CLAIMS AGAINST
 * THE UNITED STATES GOVERNMENT, ITS CONTRACTORS AND SUBCONTRACTORS, AS WELL
 * AS ANY PRIOR RECIPIENT.  IF RECIPIENT'S USE OF THE SUBJECT SOFTWARE RESULTS
 * IN ANY LIABILITIES, DEMANDS, DAMAGES, EXPENSES OR LOSSES ARISING FROM SUCH
 * USE, INCLUDING ANY DAMAGES FROM PRODUCTS BASED ON, OR RESULTING FROM,
 * RECIPIENT'S USE OF THE SUBJECT SOFTWARE, RECIPIENT SHALL INDEMNIFY AND HOLD
 * HARMLESS THE UNITED STATES GOVERNMENT, ITS CONTRACTORS AND SUBCONTRACTORS,
 * AS WELL AS ANY PRIOR RECIPIENT, TO THE EXTENT PERMITTED BY LAW.
 * RECIPIENT'S SOLE REMEDY FOR ANY SUCH MATTER SHALL BE THE IMMEDIATE,
 * UNILATERAL TERMINATION OF THIS AGREEMENT.
 *
 ******************************************************************************/

#pragma once

#include <boost/optional.hpp>
#include <crab/common/types.hpp>
#include <crab/common/stats.hpp>
#include <crab/common/bignums.hpp>
#include <crab/domains/linear_constraints.hpp>
#include <crab/domains/linear_interval_solver.hpp>
#include <crab/domains/separate_domains.hpp>
#include <crab/domains/operators_api.hpp>
#include <crab/domains/backward_assign_operations.hpp>

namespace ikos {

  template< typename Number >
  class bound;

  template< typename Number >
  class bound: public writeable {
    
    template < typename Any > friend class bound;

  public:
    typedef bound< Number > bound_t;
    
  private:
    bool _is_infinite;
    Number _n;

  private:
    bound();
    
    bound(bool is_infinite, Number n): _is_infinite(is_infinite), _n(n) {
      if (is_infinite){
        if (n > 0)
          this->_n = 1;
        else 
          this->_n = -1;
      }
    }
    
  public:
    static bound_t min(bound_t x, bound_t y) {
      return (x.operator<=(y) ? x : y);
    }

    static bound_t min(bound_t x, bound_t y, bound_t z) {
      return min(x, min(y, z));
    }

    static bound_t min(bound_t x, bound_t y, bound_t z, bound_t t) {
      return min(x, min(y, z, t));
    }

    static bound_t max(bound_t x, bound_t y) {
      return (x.operator<=(y) ? y : x);
    }

    static bound_t max(bound_t x, bound_t y, bound_t z) {
      return max(x, max(y, z));
    }

    static bound_t max(bound_t x, bound_t y, bound_t z, bound_t t) {
      return max(x, max(y, z, t));
    }

    static bound_t plus_infinity() {
      return bound_t(true, 1);
    }
    
    static bound_t minus_infinity() {
      return bound_t(true, -1);
    }
    
  public:
    bound(int n): _is_infinite(false), _n(n) { }

    bound(std::string s): _n(1) {
      if (s == "+oo") {
        this->_is_infinite = true;
      } else if (s == "-oo") {
        this->_is_infinite = true;
        this->_n = -1;
      } else {
        this->_is_infinite = false;
        this->_n = Number(s);
      }
    }

    bound(Number n): _is_infinite(false), _n(n) { }
    
    bound(const bound_t& o): writeable(), _is_infinite(o._is_infinite), _n(o._n) { }
    
    bound_t& operator=(const bound_t &o){
      if (this != &o) {
        this->_is_infinite = o._is_infinite;
        this->_n = o._n;
      }
      return *this;
    }
    
    bool is_infinite() const {
      return this->_is_infinite;
    }
    
    bool is_finite() const {
      return !this->_is_infinite;
    }

    bool is_plus_infinity() const {
      return (this->is_infinite() && this->_n > 0);
    }
    
    bool is_minus_infinity() const {
      return (this->is_infinite() && this->_n < 0);
    }
    
    bound_t operator-() const {
      return bound_t(this->_is_infinite, -this->_n);
    }
    
    bound_t operator+(bound_t x) const {
      if (this->is_finite() && x.is_finite()) {
        return bound_t(this->_n + x._n);
      } else if (this->is_finite() && x.is_infinite()) {
        return x;
      } else if (this->is_infinite() && x.is_finite()) {
        return *this;
      } else if (this->_n == x._n) {
        return *this;
      } else {
        CRAB_ERROR("Bound: undefined operation -oo + +oo");
      }
    }

    bound_t& operator+=(bound_t x)  {
      return this->operator=(this->operator+(x));
    }
		
    bound_t operator-(bound_t x) const {
      return this->operator+(x.operator-());
    }
		
    bound_t& operator-=(bound_t x)  {
      return this->operator=(this->operator-(x));
    }
    
    bound_t operator*(bound_t x) const {
      if (x._n == 0) 
        return x;
      else if (this->_n == 0)
        return *this;
      else 
        return bound_t(this->_is_infinite || x._is_infinite, this->_n * x._n);
    }
		
    bound_t& operator*=(bound_t x)  {
      return this->operator=(this->operator*(x));
    }
    
    bound_t operator/(bound_t x) const {
      if (x._n == 0) {
        CRAB_ERROR("Bound: division by zero");
      } else if (this->is_finite() && x.is_finite()) {
        return bound_t(false, _n / x._n);
      } else if (this->is_finite() && x.is_infinite()) {
        if (this->_n > 0) {
          return x;
        } else if (this->_n == 0) {
          return *this;
        } else {
          return x.operator-();
        }
      } else if (this->is_infinite() && x.is_finite()) {
        if (x._n > 0) {
          return *this;
        } else {
          return this->operator-();
        }
      } else {
        return bound_t(true, this->_n * x._n);
      }
    }
    
    bound_t& operator/=(bound_t x) {
      return this->operator=(this->operator/(x));
    }
    
    bool operator<(bound_t x) const {
      return !this->operator>=(x);
    }

    bool operator>(bound_t x) const {
      return !this->operator<=(x);
    }

    bool operator==(bound_t x) const {
      return (this->_is_infinite == x._is_infinite && this->_n == x._n);
    }
    
    bool operator!=(bound_t x) const {
      return !this->operator==(x);
    }
    
    /*	operator<= and operator>= use a somewhat optimized implementation.
     *	results include up to 20% improvements in performance in the octagon domain
     *	over a more naive implementation.
     */
    bool operator<=(bound_t x) const {
      if(this->_is_infinite xor x._is_infinite){
        if(this->_is_infinite){
          return this->_n < 0;
        }
        return x._n > 0;
      }
      return this->_n <= x._n;
    }
    
    bool operator>=(bound_t x) const {
      if(this->_is_infinite xor x._is_infinite){
        if(this->_is_infinite){
          return this->_n > 0;
        }
        return x._n < 0;
      }
      return this->_n >= x._n;
    }
    
    bound_t abs() const {
      if (this->operator>=(0)) {
        return *this;
      } else {
        return this->operator-();
      }
    }
    
    boost::optional< Number > number() const {
      if (this->is_infinite()) {
        return boost::optional< Number >();
      } else {
        return boost::optional< Number >(this->_n);
      }
    }
    
    void write(crab::crab_os& o) {
      if (this->is_plus_infinity()) {
        o << "+oo";
      } else if (this->is_minus_infinity()) {
        o << "-oo";
      } else {
        o << this->_n;
      }
    }
    
  }; // class bound

  typedef bound< z_number > z_bound;
  typedef bound< q_number > q_bound;


  namespace bounds_impl {
    // Conversion between z_bound and q_bound
    // template<class B1, class B2>
    // inline void convert_bounds(B1 b1, B2& b2);
    
    inline void convert_bounds(z_bound b1, z_bound &b2)
    { std::swap (b1,b2); }
    inline void convert_bounds(q_bound b1, q_bound &b2)
    { std::swap (b1,b2); }
    inline void convert_bounds(z_bound b1, q_bound &b2)
    {
      if (b1.is_plus_infinity())
	b2 = q_bound::plus_infinity();
      else if (b1.is_minus_infinity())
	b2 = q_bound::minus_infinity();
      else
	b2 = q_bound (q_number(*b1.number()));
    }
    inline void convert_bounds(q_bound b1, z_bound &b2)
    {
      if (b1.is_plus_infinity())
	b2 = z_bound::plus_infinity();
      else if (b1.is_minus_infinity())
	b2 = z_bound::minus_infinity();
      else
	b2 = z_bound ((*(b1.number())).round_to_lower ());
    }
  }

  
  template< typename Number >
  class interval;

  template< typename Number >
  class interval: public writeable {
    
  public:
    typedef bound< Number > bound_t;
    typedef interval< Number > interval_t;
    
  private:
    bound_t _lb;
    bound_t _ub;

  public:
    static interval_t top() {
      return interval_t(bound_t::minus_infinity(), bound_t::plus_infinity());
    }

    static interval_t bottom() {
      return interval_t();
    }

  private:
    interval(): _lb(0), _ub(-1) { }

    static Number abs(Number x) { return x < 0 ? -x : x; }
    
    static Number max(Number x, Number y) { return x.operator<=(y) ? y : x; }
    
    static Number min(Number x, Number y) { return x.operator<(y) ? x : y; }
    
  public:
    interval(bound_t lb, bound_t ub): _lb(lb), _ub(ub) { 
      if (lb > ub) {
        this->_lb = 0;
        this->_ub = -1;
      }
    }
    
    interval(bound_t b): _lb(b), _ub(b) { 
      if (b.is_infinite()) {
        this->_lb = 0;
        this->_ub = -1;	
      }
    }

    interval(Number n): _lb(n), _ub(n) { }

    interval(std::string b): _lb(b), _ub(b) { 
      if (this->_lb.is_infinite()) {
        this->_lb = 0;
        this->_ub = -1;	
      }
    }

    interval(const interval_t& i): writeable(), _lb(i._lb), _ub(i._ub) { }
    
    interval_t& operator=(interval_t i){
      this->_lb = i._lb;
      this->_ub = i._ub;
      return *this;
    }

    bound_t lb() const {
      return this->_lb;
    }

    bound_t ub() const {
      return this->_ub;
    }

    bool is_bottom() const {
      return (this->_lb > this->_ub);
    }
    
    bool is_top() const {
      return (this->_lb.is_infinite() && this->_ub.is_infinite());
    }
    
    interval_t lower_half_line() const {
      return interval_t(bound_t::minus_infinity(), this->_ub);
    }
    
    interval_t upper_half_line() const {
      return interval_t(this->_lb, bound_t::plus_infinity());
    }

    bool operator==(interval_t x) const {
      if (is_bottom()) {
        return x.is_bottom();
      } else {
        return (this->_lb == x._lb) && (this->_ub == x._ub);
      }
    }
    
    bool operator!=(interval_t x) const {
      return !this->operator==(x);
    }

    bool operator<=(interval_t x) const {
      if (this->is_bottom()) {
        return true;
      } else if (x.is_bottom()) {
        return false;
      } else {
        return (x._lb <= this->_lb) && (this->_ub <= x._ub);
      }
    }

    interval_t operator|(interval_t x) const {
      if (this->is_bottom()) {
        return x;
      } else if (x.is_bottom()) {
        return *this;
      } else {
	return interval_t(bound_t::min(this->_lb, x._lb), 
                            bound_t::max(this->_ub, x._ub));
      }
    }

    interval_t operator&(interval_t x) const {
      if (this->is_bottom() || x.is_bottom()) {
        return this->bottom();
      } else {
        return interval_t(bound_t::max(this->_lb, x._lb), 
                          bound_t::min(this->_ub, x._ub));
      }
    }
    
    interval_t operator||(interval_t x) const {
      if (this->is_bottom()) {
	return x;
      } else if (x.is_bottom()) {
	return *this;
      } else {
        return interval_t(x._lb < this->_lb ? 
                          bound_t::minus_infinity() : 
                          this->_lb, 
                          this->_ub < x._ub ?
                          bound_t::plus_infinity() : 
                          this->_ub);
      }
    }

    template<typename Thresholds>
    interval_t widening_thresholds (interval_t x, const Thresholds &ts) {
      if (this->is_bottom()) {
	return x;
      } else if (x.is_bottom()) {
	return *this;
      } else {
        bound_t lb = (x._lb < this->_lb ? ts.get_prev (x._lb) : this->_lb);
        bound_t ub = (this->_ub < x._ub ? ts.get_next (x._ub) :  this->_ub);            
        return interval_t(lb, ub);
      }
    }

    interval_t operator&&(interval_t x) const {
      if (this->is_bottom() || x.is_bottom()) {
        return this->bottom();
      } else {
        return interval_t(this->_lb.is_infinite() && x._lb.is_finite() ? 
                          x._lb : this->_lb, 
                          this->_ub.is_infinite() && x._ub.is_finite() ?
                          x._ub : this->_ub);
      }
    }

    interval_t operator+(interval_t x) const {
      if (this->is_bottom() || x.is_bottom()) {
        return this->bottom();
      } else {
	return interval_t(this->_lb + x._lb, this->_ub + x._ub);
      }
    }
    
    interval_t& operator+=(interval_t x) {
      return this->operator=(this->operator+(x));
    }
    
    interval_t operator-() const {
      if (this->is_bottom()) {
        return this->bottom();
      } else {
        return interval_t(-this->_ub, -this->_lb);
      }
    }
    
    interval_t operator-(interval_t x) const {
      if (this->is_bottom() || x.is_bottom()) {
        return this->bottom();
      } else {
        return interval_t(this->_lb - x._ub, this->_ub - x._lb);
      }
    }
    
    interval_t& operator-=(interval_t x) {
      return this->operator=(this->operator-(x));
    }
    
    interval_t operator*(interval_t x) const {
      if (this->is_bottom() || x.is_bottom()) {
        return this->bottom();
      } else {
        bound_t ll = this->_lb * x._lb;
        bound_t lu = this->_lb * x._ub;
        bound_t ul = this->_ub * x._lb;
        bound_t uu = this->_ub * x._ub;
        return interval_t(bound_t::min(ll, lu, ul, uu), 
                          bound_t::max(ll, lu, ul, uu));
      }
    }
    
    interval_t& operator*=(interval_t x) {
      return this->operator=(this->operator*(x));
    }
    
    interval_t operator/(interval_t x) const;

    interval_t& operator/=(interval_t x) {
      return this->operator=(this->operator/(x));
    }   
    
    boost::optional< Number > singleton() const {
      if (!this->is_bottom() && this->_lb == this->_ub) {
        return this->_lb.number();
      } else {
        return boost::optional< Number >();
      }
    }
    
    bool operator[](Number n) const {
      if (this->is_bottom()) {
        return false;
      } else {
        bound_t b(n);
        return (this->_lb <= b) && (b <= this->_ub);
      }
    }
    
    void write(crab::crab_os& o) {
      if (is_bottom()) {
        o << "_|_";
      } else {
        o << "[" << _lb << ", " << _ub << "]";
      }
    }    
    
    // division and remainder operations

    interval_t UDiv(interval_t x ) const {
      if (this->is_bottom() || x.is_bottom()) {
        return this->bottom();
      } else {
        return this->top();
      }
    }

    interval_t SRem(interval_t x)  const {
      if (this->is_bottom() || x.is_bottom()) {
        return this->bottom();
      } else {
        return this->top();
      }
    }

    interval_t URem(interval_t x)  const {
      if (this->is_bottom() || x.is_bottom()) {
        return this->bottom();
      } else {
        return this->top();
      }
    }

    // bitwise operations
    interval_t And(interval_t x) const {
      if (this->is_bottom() || x.is_bottom()) {
        return this->bottom();
      } else {
        return this->top();
      }
    }
    
    interval_t Or(interval_t x)  const {
      if (this->is_bottom() || x.is_bottom()) {
        return this->bottom();
      } else {
        return this->top();
      }
    }
    
    interval_t Xor(interval_t x) const { return this->Or(x); }
    
    interval_t Shl(interval_t x) const {
      if (this->is_bottom() || x.is_bottom()) {
        return this->bottom();
      } else {
        return this->top();
      }
    }

    interval_t LShr(interval_t  x) const {
      if (this->is_bottom() || x.is_bottom()) {
        return this->bottom();
      } else {
        return this->top();
      }
    }

    interval_t AShr(interval_t  x) const {
      if (this->is_bottom() || x.is_bottom()) {
        return this->bottom();
      } else {
        return this->top();
      }
    }
    
  };//  class interval

  template<>
  inline interval< q_number > interval< q_number >::
  operator/(interval< q_number > x) const {
    if (this->is_bottom() || x.is_bottom()) {
      return this->bottom();
    } else {
      boost::optional< q_number > d = x.singleton();
      if (d && *d == 0) {
        // [_, _] / 0 = _|_
        return this->bottom();
      } else if (x[0]) {
        boost::optional< q_number > n = this->singleton();
        if (n && *n == 0) {
          // 0 / [_, _] = 0
          return interval_t(q_number(0));
        } else {
          return this->top();
        }
      } else {
        bound_t ll = this->_lb / x._lb;
        bound_t lu = this->_lb / x._ub;
        bound_t ul = this->_ub / x._lb;
        bound_t uu = this->_ub / x._ub;
        return interval_t(bound_t::min(ll, lu, ul, uu), 
                          bound_t::max(ll, lu, ul, uu));
      }
    }
  }

  template<>
  inline interval< z_number > interval< z_number >::
  operator/(interval< z_number > x) const {
    if (this->is_bottom() || x.is_bottom()) {
      return this->bottom();
    } else {
      // Divisor is a singleton:
      //   the linear interval solver can perform many divisions where
      //   the divisor is a singleton interval. We optimize for this case.
      if (boost::optional<z_number> n = x.singleton()) {
	z_number c = *n;
	if (c == 1) {
	  return *this;
	} else if (c > 0) {
	  return interval_t(_lb / c, _ub / c);
	} else if (c < 0) {
	  return interval_t(_ub / c, _lb / c);
	} else {}
      }
      // Divisor is not a singleton
      typedef interval< z_number > z_interval;
      if (x[0]) {
        z_interval l(x._lb, z_bound(-1));
        z_interval u(z_bound(1), x._ub);
        return (this->operator/(l) | this->operator/(u));
      } else if (this->operator[](0)) {
        z_interval l(this->_lb, z_bound(-1));
        z_interval u(z_bound(1), this->_ub);
        return ((l / x) | (u / x) | z_interval(z_number(0)));
      } else {
        // Neither the dividend nor the divisor contains 0
        z_interval a = (this->_ub < 0) ? 
            (*this + ((x._ub < 0) ? 
                      (x + z_interval(z_number(1))) : 
                      (z_interval(z_number(1)) - x))) : *this;
	bound_t ll = a._lb / x._lb;
	bound_t lu = a._lb / x._ub;
	bound_t ul = a._ub / x._lb;
	bound_t uu = a._ub / x._ub;
	return interval_t(bound_t::min(ll, lu, ul, uu), 
			  bound_t::max(ll, lu, ul, uu));	
      }
    }
  }


  template <>
  inline interval< z_number > interval< z_number >::
  SRem(interval< z_number > x) const {
    // note that the sign of the divisor does not matter
    
    if (this->is_bottom() || x.is_bottom()) {
      return this->bottom();
    } else if (this->singleton() && x.singleton()) {
      z_number dividend = *this->singleton();
      z_number divisor = *x.singleton();
      
      if (divisor == 0) {
        return this->bottom();
      }
      
      return interval_t(dividend % divisor);
    } else if (x.ub().is_finite() && x.lb().is_finite()) {
      z_number max_divisor = max(abs(*x.lb().number()), 
                                 abs(*x.ub().number()));
      
      if (max_divisor == 0) {
        return this->bottom();
      }
      
      if (this->lb() < 0) {
        if (this->ub() > 0) {
          return interval_t(-(max_divisor - 1), max_divisor - 1);
        } else {
          return interval_t(-(max_divisor - 1), 0);
        }
      } else {
        return interval_t(0, max_divisor - 1);
      }
    } else {
      return this->top();
    }
  }

  template <>
  inline interval< z_number > interval< z_number >::
  URem(interval< z_number > x) const {
    
    if (this->is_bottom() || x.is_bottom()) {
      return this->bottom();
    } else if (this->singleton() && x.singleton()) {
      z_number dividend = *this->singleton();
      z_number divisor = *x.singleton();
      
      if (divisor < 0) {
        return this->top();
      } else if (divisor == 0) {
        return this->bottom();
      } else if (dividend < 0) {
        // dividend is treated as an unsigned integer.
        // we would need the size to be more precise
        return interval_t(0, divisor - 1);
      } else {
        return interval_t(dividend % divisor);
      }
    } else if (x.ub().is_finite() && x.lb().is_finite()) {
      z_number max_divisor = *x.ub().number();
      
      if (x.lb() < 0 || x.ub() < 0) {
        return this->top();
      } else if (max_divisor == 0) {
        return this->bottom();
      }
      
      return interval_t(0, max_divisor - 1);
    } else {
      return this->top();
    }
  }

  template <>
  inline interval< z_number > interval< z_number >::
  And(interval< z_number > x) const {
    if (this->is_bottom() || x.is_bottom()) {
      return this->bottom();
    } else {
      boost::optional< z_number > left_op = this->singleton();
      boost::optional< z_number > right_op = x.singleton();
      
      if (left_op && right_op) {
        return interval_t((*left_op) & (*right_op));
      } else if (this->lb() >= 0 && x.lb() >= 0) {
        return interval_t(0, bound_t::min(this->ub(), x.ub()));
      } else {
        return this->top();
      }
    }
  }

  template <>
  inline interval< z_number > interval< z_number >::
  Or(interval< z_number > x) const {
    if (this->is_bottom() || x.is_bottom()) {
      return this->bottom();
    } else {
      boost::optional< z_number > left_op = this->singleton();
      boost::optional< z_number > right_op = x.singleton();
      
      if (left_op && right_op) {
        return interval_t((*left_op) | (*right_op));
      } else if (this->lb() >= 0 && x.lb() >= 0) {
        boost::optional< z_number > left_ub = this->ub().number();
        boost::optional< z_number > right_ub = x.ub().number();
        
        if (left_ub && right_ub) {
          z_number m = (*left_ub > *right_ub ? *left_ub : *right_ub); 
          return interval_t(0, m.fill_ones());
        } else {
          return interval_t(0, bound_t::plus_infinity());
        }
      } else {
        return this->top();
      }
    }
  }

  template <>
  inline interval< z_number > interval< z_number >::
  Xor(interval< z_number > x)  const {
    if (this->is_bottom() || x.is_bottom()) {
      return this->bottom();
    } else {
      boost::optional< z_number > left_op = this->singleton();
      boost::optional< z_number > right_op = x.singleton();
      
      if (left_op && right_op) {
        return interval_t((*left_op) ^ (*right_op));
      } else {
        return this->Or(x);
      }
    }
  }

  template <>
  inline interval< z_number > interval< z_number >::
  Shl(interval< z_number > x) const {
    if (this->is_bottom() || x.is_bottom()) {
      return this->bottom();
    } else {
      if (boost::optional<z_number> shift = x.singleton()) {
	z_number k = *shift;
	if (k < 0) {
	  //CRAB_ERROR("lshr shift operand cannot be negative");
	  return this->top();
	}
	// Some crazy linux drivers generate shl instructions with
	// huge shifts.  We limit the number of times the loop is run
	// to avoid wasting too much time on it.
	if (k <= 128) {
	  z_number factor = 1;
	  for (int i = 0; k > i ; i++) {
	    factor *= 2;
	  }
	  return (*this) * factor;
	}
      } 
      return this->top();
    }
  }

  template <>
  inline interval< z_number > interval< z_number >::
  AShr(interval< z_number > x) const {
    if (this->is_bottom() || x.is_bottom()) {
      return this->bottom();
    } else {
      if (boost::optional<z_number> shift = x.singleton()) {
	z_number k = *shift;
	if (k < 0) {
	  //CRAB_ERROR("ashr shift operand cannot be negative");
	  return this->top();
	}	  
	// Some crazy linux drivers generate ashr instructions with
	// huge shifts.  We limit the number of times the loop is run
	// to avoid wasting too much time on it.
	if (k <= 128) {
	  z_number factor = 1;
	  for (int i = 0; k > i; i++) {
	    factor *= 2;
	  }
	  return (*this) / factor;
	}
      }
      return this->top();
    }
  }
  
  template <>
  inline interval< z_number > interval< z_number >::
  LShr(interval< z_number > x) const {
    if (this->is_bottom() || x.is_bottom()) {
      return this->bottom();
    } else {
      if (boost::optional< z_number > shift = x.singleton()) {
	z_number k = *shift;
	if (k < 0) {
	  //CRAB_ERROR("lshr shift operand cannot be negative");
	  return this->top();
	}
	// Some crazy linux drivers generate lshr instructions with
	// huge shifts.  We limit the number of times the loop is run
	// to avoid wasting too much time on it.
	if (k <= 128) {
	  if (this->lb() >= 0 && this->ub().is_finite() && shift) {
	    z_number lb = *this->lb().number();
	    z_number ub = *this->ub().number();
	    return interval< z_number >(lb >> k, ub >> k);
	  }
	}
      }
      return this->top();
    }
  }

  template< typename Number >
  inline interval< Number > operator+(Number c, interval< Number > x) {
    return interval< Number >(c) + x;
  }
  
  template< typename Number >
  inline interval< Number > operator+(interval< Number > x, Number c) {
    return x + interval< Number >(c);
  }

  template< typename Number >
  inline interval< Number > operator*(Number c, interval< Number > x) {
    return interval< Number >(c) * x;
  }

  template< typename Number >
  inline interval< Number > operator*(interval< Number > x, Number c) {
    return x * interval< Number >(c);
  }

  template< typename Number >
  inline interval< Number > operator/(Number c, interval< Number > x) {
    return interval< Number >(c) / x;
  }

  template< typename Number >
  inline interval< Number > operator/(interval< Number > x, Number c) {
    return x / interval< Number >(c);
  }

  template< typename Number >
  inline interval< Number > operator-(Number c, interval< Number > x) {
    return interval< Number >(c) - x;
  }

  template< typename Number >
  inline interval< Number > operator-(interval< Number > x, Number c) {
    return x - interval< Number >(c);
  }

  template < typename Number >
  inline crab::crab_os& operator<<(crab::crab_os& o, interval< Number > i) {
    i.write(o);
    return o;
  }

  typedef interval< z_number > z_interval;
  typedef interval< q_number > q_interval;  
  
  namespace linear_interval_solver_impl {

    template<>
    inline z_interval trim_interval(z_interval i, z_interval j) {
      if (boost::optional<z_number> c = j.singleton()) {
	if (i.lb() == *c) {
	  return z_interval(*c + 1, i.ub());
	} else if (i.ub() == *c) {
	  return z_interval(i.lb(), *c - 1);
	} else {
	}	
      }
      return i;
    }
    
    template<>
    inline q_interval trim_interval(q_interval i, q_interval /* j */) { 
      // No refinement possible for disequations over rational numbers
      return i;
    }

    template<>
    inline z_interval lower_half_line(z_interval i, bool /*is_signed*/) {
      return i.lower_half_line();
    }

    template<>
    inline q_interval lower_half_line(q_interval i, bool /*is_signed*/) {
      return i.lower_half_line();
    }

    template<>
    inline z_interval upper_half_line(z_interval i, bool /*is_signed*/) {
      return i.upper_half_line();
    }

    template<>
    inline q_interval upper_half_line(q_interval i, bool /*is_signed*/) {
      return i.upper_half_line();
    }
  } // namespace linear_interval_solver_impl

  template<typename Number, typename VariableName, std::size_t max_reduction_cycles = 10>
  class interval_domain:
    public crab::domains::
    abstract_domain<Number, VariableName,
		    interval_domain<Number,VariableName,max_reduction_cycles> >  {
  public:
    typedef interval_domain<Number, VariableName, max_reduction_cycles> interval_domain_t;
    typedef crab::domains::
    abstract_domain<Number,VariableName,interval_domain_t> abstract_domain_t;
    using typename abstract_domain_t::linear_expression_t;
    using typename abstract_domain_t::linear_constraint_t;
    using typename abstract_domain_t::linear_constraint_system_t;
    using typename abstract_domain_t::disjunctive_linear_constraint_system_t;    
    using typename abstract_domain_t::variable_t;
    using typename abstract_domain_t::number_t;
    using typename abstract_domain_t::varname_t;
    typedef interval<Number> interval_t;
    
  private:
    typedef separate_domain<variable_t, interval_t> separate_domain_t;
    typedef linear_interval_solver<Number, VariableName, separate_domain_t> solver_t;
    
  public:
    typedef typename separate_domain_t::iterator iterator;
    
  private:
    separate_domain_t _env;


    interval_domain(separate_domain_t env): _env(env) { }

  public:
    static interval_domain_t top() {
      return interval_domain(separate_domain_t::top());
    }
    
    static interval_domain_t bottom() {
      return interval_domain(separate_domain_t::bottom());
    }

  public:
    interval_domain(): _env(separate_domain_t::top()) { }

    interval_domain(const interval_domain_t& e): 
      _env(e._env) { 
      crab::CrabStats::count (getDomainName() + ".count.copy");
      crab::ScopedCrabStats __st__(getDomainName() + ".copy");
    }

    interval_domain_t& operator=(const interval_domain_t& o) {
      crab::CrabStats::count (getDomainName() + ".count.copy");
      crab::ScopedCrabStats __st__(getDomainName() + ".copy");
      if (this != &o)
        this->_env = o._env;
      return *this;
    }

    iterator begin() {
      return this->_env.begin();
    }

    iterator end() {
      return this->_env.end();
    }

    bool is_bottom() {
      return this->_env.is_bottom();
    }

    bool is_top() {
      return this->_env.is_top();
    }

    bool operator<=(interval_domain_t e) {
      crab::CrabStats::count (getDomainName() + ".count.leq");
      crab::ScopedCrabStats __st__(getDomainName() + ".leq");
      return (this->_env <= e._env);
    }

    void operator|=(interval_domain_t e) {
      crab::CrabStats::count (getDomainName() + ".count.join");
      crab::ScopedCrabStats __st__(getDomainName() + ".join");
      this->_env = this->_env | e._env;
    }

    interval_domain_t operator|(interval_domain_t e) {
      crab::CrabStats::count (getDomainName() + ".count.join");
      crab::ScopedCrabStats __st__(getDomainName() + ".join");
      return (this->_env | e._env);
    }

    interval_domain_t operator&(interval_domain_t e) {
      crab::CrabStats::count (getDomainName() + ".count.meet");
      crab::ScopedCrabStats __st__(getDomainName() + ".meet");
      return (this->_env & e._env);
    }

    interval_domain_t operator||(interval_domain_t e) {
      crab::CrabStats::count (getDomainName() + ".count.widening");
      crab::ScopedCrabStats __st__(getDomainName() + ".widening");
      return (this->_env || e._env);
    }

    template<typename Thresholds>
    interval_domain_t widening_thresholds (interval_domain_t e, const Thresholds &ts) {
      crab::CrabStats::count (getDomainName() + ".count.widening");
      crab::ScopedCrabStats __st__(getDomainName() + ".widening");
      return this->_env.widening_thresholds (e._env, ts);
    }

    interval_domain_t operator&&(interval_domain_t e) {
      crab::CrabStats::count (getDomainName() + ".count.narrowing");
      crab::ScopedCrabStats __st__(getDomainName() + ".narrowing");
      return (this->_env && e._env);
    }

    void set(variable_t v, interval_t i) {
      crab::CrabStats::count (getDomainName() + ".count.assign");
      crab::ScopedCrabStats __st__(getDomainName() + ".assign");
      this->_env.set(v, i);
    }

    void set(variable_t v, Number n) {
      crab::CrabStats::count (getDomainName() + ".count.assign");
      crab::ScopedCrabStats __st__(getDomainName() + ".assign");
      this->_env.set(v, interval_t(n));
    }

    void operator-=(variable_t v) {
      crab::CrabStats::count (getDomainName() + ".count.forget");
      crab::ScopedCrabStats __st__(getDomainName() + ".forget");
      this->_env -= v;
    }
    
    interval_t operator[](variable_t v) {
      return this->_env[v];
    }

    interval_t operator[](linear_expression_t expr) {
      interval_t r(expr.constant());
      for (typename linear_expression_t::iterator it = expr.begin(); 
           it != expr.end(); ++it) {
	interval_t c(it->first);
	r += c * this->_env[it->second];
      }
      return r;
    }
    
    void operator+=(linear_constraint_system_t csts) {
      crab::CrabStats::count (getDomainName() + ".count.add_constraints");
      crab::ScopedCrabStats __st__(getDomainName() + ".add_constraints");
      this->add(csts);
    }

    void add(linear_constraint_system_t csts, 
             std::size_t threshold = max_reduction_cycles) {
      if (!this->is_bottom()) {
	// XXX: filter out unsigned linear inequalities
	linear_constraint_system_t signed_csts;
	for (auto const& c: csts) {
	  if (c.is_inequality() && c.is_unsigned()) {
	    //CRAB_WARN("unsigned inequality skipped");
	    continue;
	  }
	  signed_csts += c;
	}
	solver_t solver(signed_csts, threshold);
	solver.run(this->_env);
      }
    }

    interval_domain_t operator+(linear_constraint_system_t csts) {
      interval_domain_t e(this->_env);
      e += csts;
      return e;
    }
    
    void assign(variable_t x, linear_expression_t e) {
      crab::CrabStats::count (getDomainName() + ".count.assign");
      crab::ScopedCrabStats __st__(getDomainName() + ".assign");

      if (boost::optional<variable_t> v = e.get_variable ()) {
        this->_env.set(x, this->_env [(*v)]);
      }
      else {
        interval_t r = e.constant();
        for (typename linear_expression_t::iterator it = e.begin(); 
             it != e.end(); ++it) {
	r += it->first * this->_env[it->second];
        }
        this->_env.set(x, r);
      }
    }

    void apply(operation_t op, variable_t x, variable_t y, variable_t z) {
      crab::CrabStats::count (getDomainName() + ".count.apply");
      crab::ScopedCrabStats __st__(getDomainName() + ".apply");

      interval_t yi = this->_env[y];
      interval_t zi = this->_env[z];
      interval_t xi = interval_t::bottom();
      
      switch (op) {
      case OP_ADDITION: {
	xi = yi + zi;
	break;
      }
      case OP_SUBTRACTION: {
	xi = yi - zi;
	break;
      }
      case OP_MULTIPLICATION: {
	xi = yi * zi;
	break;
      }
      case OP_DIVISION: {
	xi = yi / zi;
	break;
      }
      }
      this->_env.set(x, xi);
    }

    void apply(operation_t op, variable_t x, variable_t y, Number k) {
      crab::CrabStats::count (getDomainName() + ".count.apply");
      crab::ScopedCrabStats __st__(getDomainName() + ".apply");

      interval_t yi = this->_env[y];
      interval_t zi(k);
      interval_t xi = interval_t::bottom();
      
      switch (op) {
      case OP_ADDITION: {
	xi = yi + zi;
	break;
      }
      case OP_SUBTRACTION: {
	xi = yi - zi;
	break;
      }
      case OP_MULTIPLICATION: {
	xi = yi * zi;
	break;
      }
      case OP_DIVISION: {
	xi = yi / zi;
	break;
      }
      }
      this->_env.set(x, xi);
    }

    void backward_assign (variable_t x, linear_expression_t e,
			  interval_domain_t inv) {
      crab::domains::BackwardAssignOps<interval_domain_t>::
	assign (*this, x, e, inv);
    }      
    
    void backward_apply (operation_t op,
			 variable_t x, variable_t y, Number z,
			 interval_domain_t inv) {
      crab::domains::BackwardAssignOps<interval_domain_t>::
	apply(*this, op, x, y, z, inv);
    }      
    
    void backward_apply(operation_t op,
			variable_t x, variable_t y, variable_t z,
			interval_domain_t inv) {
      crab::domains::BackwardAssignOps<interval_domain_t>::
	apply(*this, op, x, y, z, inv);
    }
    
    // cast_operators_api
    
    void apply(crab::domains::int_conv_operation_t /*op*/,
	       variable_t dst, variable_t src){
      // ignore the widths 
      assign(dst, src);
    }

    // bitwise_operators_api
    
    void apply(bitwise_operation_t op, variable_t x, variable_t y, variable_t z){
      crab::CrabStats::count (getDomainName() + ".count.apply");
      crab::ScopedCrabStats __st__(getDomainName() + ".apply");

      interval_t yi = this->_env[y];
      interval_t zi = this->_env[z];
      interval_t xi = interval_t::bottom();
      
      switch (op) {
        case OP_AND: {
	xi = yi.And(zi);
	break;
        }
        case OP_OR: {
	xi = yi.Or(zi);
	break;
        }
        case OP_XOR: {
	xi = yi.Xor(zi);
	break;
        }
        case OP_SHL: {
	xi = yi.Shl(zi);
	break;
        }
        case OP_LSHR: {
          xi = yi.LShr(zi);
          break;
        }
        case OP_ASHR: {
	xi = yi.AShr(zi);
	break;
        }
        default: 
          CRAB_ERROR("unreachable");
      }
      this->_env.set(x, xi);
    }
    
    void apply(bitwise_operation_t op, variable_t x, variable_t y, Number k){
      crab::CrabStats::count (getDomainName() + ".count.apply");
      crab::ScopedCrabStats __st__(getDomainName() + ".apply");

      interval_t yi = this->_env[y];
      interval_t zi(k);
      interval_t xi = interval_t::bottom();
      switch (op) {
        case OP_AND: {
	xi = yi.And(zi);
	break;
        }
        case OP_OR: {
	xi = yi.Or(zi);
	break;
        }
        case OP_XOR: {
	xi = yi.Xor(zi);
	break;
        }
        case OP_SHL: {
	xi = yi.Shl(zi);
	break;
        }
        case OP_LSHR: {
          xi = yi.LShr(zi);
          break;
        }
        case OP_ASHR: {
	xi = yi.AShr(zi);
	break;
        }
        default: 
          CRAB_ERROR("unreachable");
      }
      this->_env.set(x, xi);
    }
    
    // division_operators_api
    
    void apply(div_operation_t op, variable_t x, variable_t y, variable_t z){
      crab::CrabStats::count (getDomainName() + ".count.apply");
      crab::ScopedCrabStats __st__(getDomainName() + ".apply");

      interval_t yi = this->_env[y];
      interval_t zi = this->_env[z];
      interval_t xi = interval_t::bottom();
      
      switch (op) {
        case OP_SDIV: {
	xi = yi / zi;
	break;
        }
        case OP_UDIV: {
	xi = yi.UDiv(zi);
	break;
        }
        case OP_SREM: {
	xi = yi.SRem(zi);
	break;
        }
        case OP_UREM: {
	xi = yi.URem(zi);
	break;
        }
        default: 
          CRAB_ERROR("unreachable");
      }
      this->_env.set(x, xi);

    }

    void apply(div_operation_t op, variable_t x, variable_t y, Number k){
      crab::CrabStats::count (getDomainName() + ".count.apply");
      crab::ScopedCrabStats __st__(getDomainName() + ".apply");

      interval_t yi = this->_env[y];
      interval_t zi(k);
      interval_t xi = interval_t::bottom();
      switch (op) {
        case OP_SDIV: {
	xi = yi / zi;
	break;
        }
        case OP_UDIV: {
	xi = yi.UDiv(zi);
	break;
        }
        case OP_SREM: {
	xi = yi.SRem(zi);
	break;
        }
        case OP_UREM: {
	xi = yi.URem(zi);
	break;
        }
        default: 
          CRAB_ERROR("unreachable");
      }
      this->_env.set(x, xi);
    }

    void write(crab::crab_os& o) {
      this->_env.write(o);
    }

    linear_constraint_system_t to_linear_constraint_system() {
      linear_constraint_system_t csts;
      
      if (this->is_bottom()) {
        csts += linear_constraint_t::get_false();
        return csts;
      }

      for (iterator it = this->_env.begin(); it != this->_env.end(); ++it) {
        variable_t v = it->first;
        interval_t   i = it->second;
        boost::optional<Number> lb = i.lb().number();
        boost::optional<Number> ub = i.ub().number();
        if (lb) csts += linear_constraint_t(v >= *lb);
        if (ub) csts += linear_constraint_t(v <= *ub);
      }
      return csts;
    }

    disjunctive_linear_constraint_system_t to_disjunctive_linear_constraint_system() {
      auto lin_csts = to_linear_constraint_system();
      if (lin_csts.is_false()) {
	return disjunctive_linear_constraint_system_t(true /*is_false*/); 
      } else if (lin_csts.is_true()) {
	return disjunctive_linear_constraint_system_t(false /*is_false*/);
      } else {
	return disjunctive_linear_constraint_system_t(lin_csts);
      }
    }
    
    static std::string getDomainName () {
      return "Intervals";
    }

  }; // class interval_domain
  
} // namespace ikos


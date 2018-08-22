#ifndef JIT_FE_C_H
#define JIT_FE_C_H

namespace jit {
  namespace {
    constexpr std::size_t increment(bool)          { return 1; };
    constexpr std::size_t increment(std::int8_t  ) { return 1; };
    constexpr std::size_t increment(std::int16_t ) { return 1; };
    constexpr std::size_t increment(std::int32_t ) { return 1; };
    constexpr std::size_t increment(std::int64_t ) { return 1; };
    constexpr std::size_t increment(std::uint8_t ) { return 1; };
    constexpr std::size_t increment(std::uint16_t) { return 1; };
    constexpr std::size_t increment(std::uint32_t) { return 1; };
    constexpr std::size_t increment(std::uint64_t) { return 1; };
    template<typename U> constexpr std::size_t increment(U *) { return sizeof(U); };
  }

  template<typename T> class Value {
  public:
    Value(fe::Generator *gen, T val) : node(std::make_shared<fe::Constant>(gen, fe::type_to_id(T()), val)) { }
    Value(std::shared_ptr<fe::Node> _node) : node(std::move(_node)) {}
    Value(const Value<T> &src) : node(src.node) { }
    template<typename U> Value(const Value<U> &src, std::enable_if_t<!std::is_same<T, U>::value && fe::is_handled_integral<U>::value && fe::is_handled_integral<T>::value, int> = 0) : node(std::make_shared<fe::Convert>(src.node->gen, fe::type_to_id(T()), fe::type_to_id(U()), src.node)) {}

    template<typename U> std::enable_if_t<std::is_same<T, U>::value, Value<T> &> operator = (const Value<U> &src) { reassign(src.node); return *this; }
    template<typename U> std::enable_if_t<!std::is_same<T, U>::value, Value<T>> operator = (const Value<U> &src) { reassign(std::make_shared<fe::Convert>(src.node->gen, fe::type_to_id(T()), fe::type_to_id(U()), src.node)); return *this; }

    template<typename U> Value<T> operator |=(const Value<U> &src) { Value<T> s1(src); reassign(std::make_shared<fe::BiOp>(node->gen, fe::type_to_id(T()), fe::BIOP_BOR, node, s1.node)); return *this; }
    template<typename U> Value<T> operator &=(const Value<U> &src) { Value<T> s1(src); reassign(std::make_shared<fe::BiOp>(node->gen, fe::type_to_id(T()), fe::BIOP_BAND, node, s1.node)); return *this; }
    template<typename U> Value<T> operator ^=(const Value<U> &src) { Value<T> s1(src); reassign(std::make_shared<fe::BiOp>(node->gen, fe::type_to_id(T()), fe::BIOP_BXOR, node, s1.node)); return *this; }
    template<typename U> Value<T> operator +=(const Value<U> &src) {
      Value<T> s1(src);
      reassign(std::make_shared<fe::BiOp>(node->gen, fe::type_to_id(T()), fe::BIOP_ADD, node, s1.node));
      return *this;
    }
    template<typename U> Value<T> operator -=(const Value<U> &src) { Value<T> s1(src); reassign(std::make_shared<fe::BiOp>(node->gen, fe::type_to_id(T()), fe::BIOP_SUB, node, s1.node)); return *this; }

    Value<T> operator ++() {
      std::size_t inc = increment(T());
      reassign(std::make_shared<fe::BiOp>(node->gen, fe::type_to_id(T()), fe::BIOP_ADD, node, std::make_shared<fe::Constant>(node->gen, fe::type_to_id(T()), inc)));
      return *this;
    }

    Value<T> operator ++(int) {
      std::size_t inc = increment(T());
      Value<T> res(node);
      reassign(std::make_shared<fe::BiOp>(node->gen, fe::type_to_id(T()), fe::BIOP_ADD, node, std::make_shared<fe::Constant>(node->gen, fe::type_to_id(T()), inc)));
      return res;
    }

    Value<T> operator --() {
      std::size_t inc = increment(T());
      reassign(std::make_shared<fe::BiOp>(node->gen, fe::type_to_id(T()), fe::BIOP_SUB, node, std::make_shared<fe::Constant>(node->gen, fe::type_to_id(T()), inc)));
      return *this;
    }

    Value<T> operator --(int) {
      std::size_t inc = increment(T());
      Value<T> res(node);
      reassign(std::make_shared<fe::BiOp>(node->gen, fe::type_to_id(T()), fe::BIOP_SUB, node, std::make_shared<fe::Constant>(node->gen, fe::type_to_id(T()), inc)));
      return res;
    }

    Value<T>    operator ~() { return Value<T>(std::make_shared<fe::UnOp>(node->gen, fe::type_to_id(T()), fe::UNOP_NEG, node)); }
    Value<bool> operator !() { return Value<bool>(std::make_shared<fe::UnOp>(node->gen, fe::type_to_id(T()), fe::UNOP_NOT, node)); }

    template<typename U = typename std::remove_pointer<T>::type> std::enable_if_t<std::is_same<U *, T>::value, Value<U>> operator *() {
      return Value<U>(std::make_shared<fe::UnOp>(node->gen, fe::type_to_id(U()), fe::UNOP_DEREF, node));
    }

    template<typename U> Value<U> ternary(Value<U> yes, Value<U> no) {
      return Value<U>(std::make_shared<fe::Ternary>(node->gen, fe::type_to_id(U()), node, yes(), no()));
    }

    template<typename U> std::enable_if_t<fe::is_handled_integral<U>::value, Value<U>> ternary(Value<U> yes, U no) {
      return Value<U>(std::make_shared<fe::Ternary>(node->gen, fe::type_to_id(U()), node, yes(), std::make_shared<fe::Constant>(node->gen, fe::type_to_id(U()), no)));
    }

    template<typename U> std::enable_if_t<fe::is_handled_integral<U>::value, Value<U>> ternary(U yes, Value<U> no) {
      return Value<U>(std::make_shared<fe::Ternary>(node->gen, fe::type_to_id(U()), node, std::make_shared<fe::Constant>(node->gen, fe::type_to_id(U()), yes), no()));
    }

    template<typename U> std::enable_if_t<fe::is_handled_integral<U>::value, Value<U>> ternary(U yes, U no) {
      return Value<U>(std::make_shared<fe::Ternary>(node->gen, fe::type_to_id(U()), node, std::make_shared<fe::Constant>(node->gen, fe::type_to_id(U()), yes), std::make_shared<fe::Constant>(node->gen, fe::type_to_id(U()), no)));
    }

    template<typename U> Value<U> member(std::size_t offset) {
      return Value<U>(std::make_shared<fe::UnOp>(node->gen, fe::type_to_id(T()), fe::UNOP_DEREF, std::make_shared<fe::BiOp>(node->gen, fe::POINTER, fe::BIOP_ADD, node, std::make_shared<fe::Constant>(node->gen, fe::type_to_id(std::size_t()), offset))));
    }

    std::shared_ptr<fe::Node> operator()() { return node; }

  private:
    template<typename U> friend class Value;
    std::shared_ptr<fe::Node> node;

    void reassign(std::shared_ptr<fe::Node> _node) { node = _node; }
  };

  template<typename U, typename V> Value<typename fe::binary_promotion<U, V>::type> operator &(Value<U> a, Value<V> b) { using W = typename fe::binary_promotion<U, V>::type; Value<W> a1(a), b1(b); return Value<W>(std::make_shared<fe::BiOp>(a()->gen, fe::type_to_id(W()), fe::BIOP_BAND, a1(), b1())); }
  template<typename U, typename V> Value<typename fe::binary_promotion<U, V>::type> operator |(Value<U> a, Value<V> b) { using W = typename fe::binary_promotion<U, V>::type; Value<W> a1(a), b1(b); return Value<W>(std::make_shared<fe::BiOp>(a()->gen, fe::type_to_id(W()), fe::BIOP_BOR, a1(), b1())); }
  template<typename U, typename V> Value<typename fe::binary_promotion<U, V>::type> operator ^(Value<U> a, Value<V> b) { using W = typename fe::binary_promotion<U, V>::type; Value<W> a1(a), b1(b); return Value<W>(std::make_shared<fe::BiOp>(a()->gen, fe::type_to_id(W()), fe::BIOP_BXOR, a1(), b1())); }
  template<typename U, typename V> Value<typename fe::binary_promotion<U, V>::type> operator +(Value<U> a, Value<V> b) { using W = typename fe::binary_promotion<U, V>::type; Value<W> a1(a), b1(b); return Value<W>(std::make_shared<fe::BiOp>(a()->gen, fe::type_to_id(W()), fe::BIOP_ADD, a1(), b1())); }
  template<typename U, typename V> Value<typename fe::binary_promotion<U, V>::type> operator -(Value<U> a, Value<V> b) { using W = typename fe::binary_promotion<U, V>::type; Value<W> a1(a), b1(b); return Value<W>(std::make_shared<fe::BiOp>(a()->gen, fe::type_to_id(W()), fe::BIOP_SUB, a1(), b1())); }
  template<typename U, typename V> Value<typename fe::binary_promotion<U, V>::type> operator *(Value<U> a, Value<V> b) { using W = typename fe::binary_promotion<U, V>::type; Value<W> a1(a), b1(b); return Value<W>(std::make_shared<fe::BiOp>(a()->gen, fe::type_to_id(W()), fe::BIOP_MUL, a1(), b1())); }
  template<typename U, typename V> Value<typename fe::binary_promotion<U, V>::type> operator /(Value<U> a, Value<V> b) { using W = typename fe::binary_promotion<U, V>::type; Value<W> a1(a), b1(b); return Value<W>(std::make_shared<fe::BiOp>(a()->gen, fe::type_to_id(W()), fe::BIOP_DIV, a1(), b1())); }

  template<typename U, typename V> Value<typename fe::binary_promotion<U, V>::type> operator &(Value<U> a, V b) { using W = typename fe::binary_promotion<U, V>::type; Value<W> a1(a), b1(a()->gen, b); return Value<W>(std::make_shared<fe::BiOp>(a()->gen, fe::type_to_id(W()), fe::BIOP_BAND, a1(), b1())); }
  template<typename U, typename V> Value<typename fe::binary_promotion<U, V>::type> operator |(Value<U> a, V b) { using W = typename fe::binary_promotion<U, V>::type; Value<W> a1(a), b1(a()->gen, b); return Value<W>(std::make_shared<fe::BiOp>(a()->gen, fe::type_to_id(W()), fe::BIOP_BOR, a1(), b1())); }
  template<typename U, typename V> Value<typename fe::binary_promotion<U, V>::type> operator ^(Value<U> a, V b) { using W = typename fe::binary_promotion<U, V>::type; Value<W> a1(a), b1(a()->gen, b); return Value<W>(std::make_shared<fe::BiOp>(a()->gen, fe::type_to_id(W()), fe::BIOP_BXOR, a1(), b1())); }
  template<typename U, typename V> Value<typename fe::binary_promotion<U, V>::type> operator +(Value<U> a, V b) { using W = typename fe::binary_promotion<U, V>::type; Value<W> a1(a), b1(a()->gen, b); return Value<W>(std::make_shared<fe::BiOp>(a()->gen, fe::type_to_id(W()), fe::BIOP_ADD, a1(), b1())); }
  template<typename U, typename V> Value<typename fe::binary_promotion<U, V>::type> operator -(Value<U> a, V b) { using W = typename fe::binary_promotion<U, V>::type; Value<W> a1(a), b1(a()->gen, b); return Value<W>(std::make_shared<fe::BiOp>(a()->gen, fe::type_to_id(W()), fe::BIOP_SUB, a1(), b1())); }
  template<typename U, typename V> Value<typename fe::binary_promotion<U, V>::type> operator *(Value<U> a, V b) { using W = typename fe::binary_promotion<U, V>::type; Value<W> a1(a), b1(a()->gen, b); return Value<W>(std::make_shared<fe::BiOp>(a()->gen, fe::type_to_id(W()), fe::BIOP_MUL, a1(), b1())); }
  template<typename U, typename V> Value<typename fe::binary_promotion<U, V>::type> operator /(Value<U> a, V b) { using W = typename fe::binary_promotion<U, V>::type; Value<W> a1(a), b1(a()->gen, b); return Value<W>(std::make_shared<fe::BiOp>(a()->gen, fe::type_to_id(W()), fe::BIOP_DIV, a1(), b1())); }

  template<typename U, typename V> Value<typename fe::binary_promotion<U, V>::type> operator &(U a, Value<V> b) { using W = typename fe::binary_promotion<U, V>::type; Value<W> a1(b()->gen, a), b1(b); return Value<W>(std::make_shared<fe::BiOp>(b()->gen, fe::type_to_id(W()), fe::BIOP_BAND, a1(), b1())); }
  template<typename U, typename V> Value<typename fe::binary_promotion<U, V>::type> operator |(U a, Value<V> b) { using W = typename fe::binary_promotion<U, V>::type; Value<W> a1(b()->gen, a), b1(b); return Value<W>(std::make_shared<fe::BiOp>(b()->gen, fe::type_to_id(W()), fe::BIOP_BOR, a1(), b1())); }
  template<typename U, typename V> Value<typename fe::binary_promotion<U, V>::type> operator ^(U a, Value<V> b) { using W = typename fe::binary_promotion<U, V>::type; Value<W> a1(b()->gen, a), b1(b); return Value<W>(std::make_shared<fe::BiOp>(b()->gen, fe::type_to_id(W()), fe::BIOP_BXOR, a1(), b1())); }
  template<typename U, typename V> Value<typename fe::binary_promotion<U, V>::type> operator +(U a, Value<V> b) { using W = typename fe::binary_promotion<U, V>::type; Value<W> a1(b()->gen, a), b1(b); return Value<W>(std::make_shared<fe::BiOp>(b()->gen, fe::type_to_id(W()), fe::BIOP_ADD, a1(), b1())); }
  template<typename U, typename V> Value<typename fe::binary_promotion<U, V>::type> operator -(U a, Value<V> b) { using W = typename fe::binary_promotion<U, V>::type; Value<W> a1(b()->gen, a), b1(b); return Value<W>(std::make_shared<fe::BiOp>(b()->gen, fe::type_to_id(W()), fe::BIOP_SUB, a1(), b1())); }
  template<typename U, typename V> Value<typename fe::binary_promotion<U, V>::type> operator *(U a, Value<V> b) { using W = typename fe::binary_promotion<U, V>::type; Value<W> a1(b()->gen, a), b1(b); return Value<W>(std::make_shared<fe::BiOp>(b()->gen, fe::type_to_id(W()), fe::BIOP_MUL, a1(), b1())); }
  template<typename U, typename V> Value<typename fe::binary_promotion<U, V>::type> operator /(U a, Value<V> b) { using W = typename fe::binary_promotion<U, V>::type; Value<W> a1(b()->gen, a), b1(b); return Value<W>(std::make_shared<fe::BiOp>(b()->gen, fe::type_to_id(W()), fe::BIOP_DIV, a1(), b1())); }

  template<typename U, typename V> Value<bool> operator &&(Value<U> a, Value<V> b) { return Value<bool>(std::make_shared<fe::BiOp>(a()->gen, fe::BOOL, fe::BIOP_LAND, a(), b())); }
  template<typename U, typename V> Value<bool> operator ||(Value<U> a, Value<V> b) { return Value<bool>(std::make_shared<fe::BiOp>(a()->gen, fe::BOOL, fe::BIOP_LOR, a(), b())); }
  template<typename U, typename V> Value<bool> operator <=(Value<U> a, Value<V> b) { using W = typename fe::binary_promotion<U, V>::type; Value<W> a1(a), b1(b); return Value<bool>(std::make_shared<fe::BiOp>(a()->gen, fe::BOOL, fe::BIOP_LE, a1(), b1())); }
  template<typename U, typename V> Value<bool> operator < (Value<U> a, Value<V> b) { using W = typename fe::binary_promotion<U, V>::type; Value<W> a1(a), b1(b); return Value<bool>(std::make_shared<fe::BiOp>(a()->gen, fe::BOOL, fe::BIOP_LT, a1(), b1())); }
  template<typename U, typename V> Value<bool> operator >=(Value<U> a, Value<V> b) { using W = typename fe::binary_promotion<U, V>::type; Value<W> a1(a), b1(b); return Value<bool>(std::make_shared<fe::BiOp>(a()->gen, fe::BOOL, fe::BIOP_GE, a1(), b1())); }
  template<typename U, typename V> Value<bool> operator > (Value<U> a, Value<V> b) { using W = typename fe::binary_promotion<U, V>::type; Value<W> a1(a), b1(b); return Value<bool>(std::make_shared<fe::BiOp>(a()->gen, fe::BOOL, fe::BIOP_GT, a1(), b1())); }
  template<typename U, typename V> Value<bool> operator ==(Value<U> a, Value<V> b) { using W = typename fe::binary_promotion<U, V>::type; Value<W> a1(a), b1(b); return Value<bool>(std::make_shared<fe::BiOp>(a()->gen, fe::BOOL, fe::BIOP_EQ, a1(), b1())); }
  template<typename U, typename V> Value<bool> operator !=(Value<U> a, Value<V> b) { using W = typename fe::binary_promotion<U, V>::type; Value<W> a1(a), b1(b); return Value<bool>(std::make_shared<fe::BiOp>(a()->gen, fe::BOOL, fe::BIOP_NE, a1(), b1())); }

  template<typename U, typename V> Value<bool> operator &&(Value<U> a, V b) { return Value<bool>(std::make_shared<fe::BiOp>(a()->gen, fe::BOOL, fe::BIOP_LAND, a(), std::make_shared<fe::Constant>(a()->gen, fe::type_to_id(V()), b))); }
  template<typename U, typename V> Value<bool> operator ||(Value<U> a, V b) { return Value<bool>(std::make_shared<fe::BiOp>(a()->gen, fe::BOOL, fe::BIOP_LOR, a(), std::make_shared<fe::Constant>(a()->gen, fe::type_to_id(V()), b))); }
  template<typename U, typename V> Value<bool> operator <=(Value<U> a, V b) { using W = typename fe::binary_promotion<U, V>::type; Value<W> a1(a), b1(a()->gen, b); return Value<bool>(std::make_shared<fe::BiOp>(a()->gen, fe::BOOL, fe::BIOP_LE, a1(), b1())); }
  template<typename U, typename V> Value<bool> operator < (Value<U> a, V b) { using W = typename fe::binary_promotion<U, V>::type; Value<W> a1(a), b1(a()->gen, b); return Value<bool>(std::make_shared<fe::BiOp>(a()->gen, fe::BOOL, fe::BIOP_LT, a1(), b1())); }
  template<typename U, typename V> Value<bool> operator >=(Value<U> a, V b) { using W = typename fe::binary_promotion<U, V>::type; Value<W> a1(a), b1(a()->gen, b); return Value<bool>(std::make_shared<fe::BiOp>(a()->gen, fe::BOOL, fe::BIOP_GE, a1(), b1())); }
  template<typename U, typename V> Value<bool> operator > (Value<U> a, V b) { using W = typename fe::binary_promotion<U, V>::type; Value<W> a1(a), b1(a()->gen, b); return Value<bool>(std::make_shared<fe::BiOp>(a()->gen, fe::BOOL, fe::BIOP_GT, a1(), b1())); }
  template<typename U, typename V> Value<bool> operator ==(Value<U> a, V b) { using W = typename fe::binary_promotion<U, V>::type; Value<W> a1(a), b1(a()->gen, b); return Value<bool>(std::make_shared<fe::BiOp>(a()->gen, fe::BOOL, fe::BIOP_EQ, a1(), b1())); }
  template<typename U, typename V> Value<bool> operator !=(Value<U> a, V b) { using W = typename fe::binary_promotion<U, V>::type; Value<W> a1(a), b1(a()->gen, b); return Value<bool>(std::make_shared<fe::BiOp>(a()->gen, fe::BOOL, fe::BIOP_NE, a1(), b1())); }

  template<typename U, typename V> Value<bool> operator &&(U a, Value<V> b) { return Value<bool>(std::make_shared<fe::BiOp>(b()->gen, fe::BOOL, fe::BIOP_LAND, std::make_shared<fe::Constant>(b()->gen, fe::type_to_id(U()), a), b())); }
  template<typename U, typename V> Value<bool> operator ||(U a, Value<V> b) { return Value<bool>(std::make_shared<fe::BiOp>(b()->gen, fe::BOOL, fe::BIOP_LOR, std::make_shared<fe::Constant>(b()->gen, fe::type_to_id(U()), a), b())); }
  template<typename U, typename V> Value<bool> operator <=(U a, Value<V> b) { using W = typename fe::binary_promotion<U, V>::type; Value<W> a1(b()->gen, a), b1(b); return Value<bool>(std::make_shared<fe::BiOp>(b()->gen, fe::BOOL, fe::BIOP_LE, a1(), b1())); }
  template<typename U, typename V> Value<bool> operator < (U a, Value<V> b) { using W = typename fe::binary_promotion<U, V>::type; Value<W> a1(b()->gen, a), b1(b); return Value<bool>(std::make_shared<fe::BiOp>(b()->gen, fe::BOOL, fe::BIOP_LT, a1(), b1())); }
  template<typename U, typename V> Value<bool> operator >=(U a, Value<V> b) { using W = typename fe::binary_promotion<U, V>::type; Value<W> a1(b()->gen, a), b1(b); return Value<bool>(std::make_shared<fe::BiOp>(b()->gen, fe::BOOL, fe::BIOP_GE, a1(), b1())); }
  template<typename U, typename V> Value<bool> operator > (U a, Value<V> b) { using W = typename fe::binary_promotion<U, V>::type; Value<W> a1(b()->gen, a), b1(b); return Value<bool>(std::make_shared<fe::BiOp>(b()->gen, fe::BOOL, fe::BIOP_GT, a1(), b1())); }
  template<typename U, typename V> Value<bool> operator ==(U a, Value<V> b) { using W = typename fe::binary_promotion<U, V>::type; Value<W> a1(b()->gen, a), b1(b); return Value<bool>(std::make_shared<fe::BiOp>(b()->gen, fe::BOOL, fe::BIOP_EQ, a1(), b1())); }
  template<typename U, typename V> Value<bool> operator !=(U a, Value<V> b) { using W = typename fe::binary_promotion<U, V>::type; Value<W> a1(b()->gen, a), b1(b); return Value<bool>(std::make_shared<fe::BiOp>(b()->gen, fe::BOOL, fe::BIOP_NE, a1(), b1())); }

  template<typename U, typename V> std::enable_if_t<fe::is_handled_integral<U>::value && fe::is_handled_integral<V>::value, Value<U>> operator <<(Value<U> a, Value<V> b) {
    return Value<U>(std::make_shared<fe::BiOp>(a()->gen, fe::type_to_id(U()), fe::BIOP_LSHIFT, a(), b()));
  }

  template<typename U, typename V> std::enable_if_t<fe::is_handled_integral<U>::value && fe::is_handled_integral<V>::value, Value<U>> operator <<(Value<U> a, V b) {
    return Value<U>(std::make_shared<fe::BiOp>(a()->gen, fe::type_to_id(U()), fe::BIOP_LSHIFT, a(), std::make_shared<fe::Constant>(a()->gen, fe::type_to_id(V()), b)));
  }

  template<typename U, typename V> std::enable_if_t<fe::is_handled_integral<U>::value && fe::is_handled_integral<V>::value, Value<U>> operator <<(U a, Value<V> b) {
    return Value<U>(std::make_shared<fe::BiOp>(b()->gen, fe::type_to_id(U()), fe::BIOP_LSHIFT, std::make_shared<fe::Constant>(b()->gen, fe::type_to_id(U()), a), b()));
  }

  template<typename U, typename V> std::enable_if_t<fe::is_handled_integral<U>::value && fe::is_handled_integral<V>::value, Value<U>> operator >>(Value<U> a, Value<V> b) {
    return Value<U>(std::make_shared<fe::BiOp>(a()->gen, fe::type_to_id(U()), fe::BIOP_RSHIFT, a(), b()));
  }

  template<typename U, typename V> std::enable_if_t<fe::is_handled_integral<U>::value && fe::is_handled_integral<V>::value, Value<U>> operator >>(Value<U> a, V b) {
    return Value<U>(std::make_shared<fe::BiOp>(a()->gen, fe::type_to_id(U()), fe::BIOP_RSHIFT, a(), std::make_shared<fe::Constant>(a()->gen, fe::type_to_id(V()), b)));
  }

  template<typename U, typename V> std::enable_if_t<fe::is_handled_integral<U>::value && fe::is_handled_integral<V>::value, Value<U>> operator >>(U a, Value<V> b) {
    return Value<U>(std::make_shared<fe::BiOp>(b()->gen, fe::type_to_id(U()), fe::BIOP_RSHIFT, std::make_shared<fe::Constant>(b()->gen, fe::type_to_id(U()), a), b()));
  }

  template<typename U, typename V> std::enable_if_t<fe::is_handled_integral<V>::value, Value<U *>> operator +(Value<U *> a, Value<V> b) {
    Value<std::size_t> b1(b);
    return Value<U *>(std::make_shared<fe::BiOp>(a()->gen, fe::POINTER, fe::BIOP_ADD, a(), std::make_shared<fe::BiOp>(a()->gen, fe::type_to_id(std::size_t()), fe::BIOP_MUL, std::make_shared<fe::Constant>(a()->gen, fe::type_to_id(std::size_t()), sizeof(U)), b1())));
  }

  template<typename U, typename V> std::enable_if_t<fe::is_handled_integral<V>::value, Value<U *>> operator +(Value<U *> a, V b) {
    return Value<U *>(std::make_shared<fe::BiOp>(a()->gen, fe::POINTER, fe::BIOP_ADD, a(), std::make_shared<fe::Constant>(a()->gen, fe::type_to_id(std::size_t()), b*sizeof(U))));
  }

  template<typename U, typename V> std::enable_if_t<fe::is_handled_integral<V>::value, Value<U *>> operator -(Value<U *> a, Value<V> b) {
    Value<std::size_t> b1(b);
    return Value<U *>(std::make_shared<fe::BiOp>(a()->gen, fe::POINTER, fe::BIOP_SUB, a(), std::make_shared<fe::BiOp>(a()->gen, fe::type_to_id(std::size_t()), fe::BIOP_MUL, std::make_shared<fe::Constant>(a()->gen, fe::type_to_id(std::size_t()), sizeof(U)), b1())));
  }

  template<typename U, typename V> std::enable_if_t<fe::is_handled_integral<V>::value, Value<U *>> operator -(Value<U *> a, V b) {
    return Value<U *>(std::make_shared<fe::BiOp>(a()->gen, fe::POINTER, fe::BIOP_SUB, a(), std::make_shared<fe::Constant>(a()->gen, fe::type_to_id(std::size_t()), b*sizeof(U))));
  }



  template<typename T, int size> class Array1D {
  public:
    Array1D(std::shared_ptr<fe::Node> _node) : node(std::move(_node)) { }
    template<typename U> std::enable_if_t<fe::is_handled_integral<U>::value, Value<T>> operator[](U entry) {
      return Value<T>(std::make_shared<fe::UnOp>(node->gen, fe::type_to_id(T()), fe::UNOP_DEREF, std::make_shared<fe::BiOp>(node->gen, fe::POINTER, fe::BIOP_ADD, node, std::make_shared<fe::Constant>(node->gen, fe::type_to_id(std::size_t()), entry*sizeof(T)))));
    }

    template<typename U> std::enable_if_t<fe::is_handled_integral<U>::value, Value<T>> operator[](Value<U> entry) {
      Value<std::size_t> e1(entry);
      return Value<T>(std::make_shared<fe::UnOp>(node->gen, fe::type_to_id(T()), fe::UNOP_DEREF, std::make_shared<fe::BiOp>(node->gen, fe::POINTER, fe::BIOP_ADD, node, std::make_shared<fe::BiOp>(node->gen, fe::type_to_id(std::size_t()), fe::BIOP_MUL, std::make_shared<fe::Constant>(node->gen, fe::type_to_id(std::size_t()), sizeof(T)), e1()))));
    }

  private:
    std::shared_ptr<fe::Node> node;
  };

  template<typename T, int size1, int size2> class Array2D {
  public:
    Array2D(std::shared_ptr<fe::Node> _node) : node(std::move(_node)) { }
    template<typename U> std::enable_if_t<fe::is_handled_integral<U>::value, Array1D<T, size2>> operator[](U entry) {
      return Array1D<T, size2>(std::make_shared<fe::BiOp>(node->gen, fe::POINTER, fe::BIOP_ADD, node, std::make_shared<fe::Constant>(node->gen, fe::type_to_id(std::size_t()), entry*size1*sizeof(T))));
    }

    template<typename U> std::enable_if_t<fe::is_handled_integral<U>::value, Array1D<T, size2>> operator[](Value<U> entry) {
      Value<std::size_t> e1(entry);
      return Array1D<T, size2>(std::make_shared<fe::BiOp>(node->gen, fe::POINTER, fe::BIOP_ADD, node, std::make_shared<fe::BiOp>(node->gen, fe::type_to_id(std::size_t()), fe::BIOP_MUL, std::make_shared<fe::Constant>(node->gen, fe::type_to_id(std::size_t()), size1*sizeof(T)), e1())));
    }


  private:
    std::shared_ptr<fe::Node> node;
  };

  class Function : public fe::Generator {
  public:
    Function(Context &context);

    template<typename T> Value<T> mk_var(std::string name) {
      auto node(std::make_shared<fe::Variable>(this, name, fe::type_to_id(T()), nullptr));
      vars.emplace_back(node);
      return Value<T>(node);
    }

    template<typename T, typename U = T> Value<T> mk_var(std::string name, Value<U> value) {
      auto node(std::make_shared<fe::Variable>(this, name, fe::type_to_id(T()), value()));
      vars.emplace_back(node);
      return Value<T>(node);
    }

    template<typename T> Value<T> mk_var(std::string name, T value) {
      auto node(std::make_shared<fe::Variable>(this, name, fe::type_to_id(T()), std::make_shared<fe::Constant>(this, fe::type_to_id(T()), value)));
      vars.emplace_back(node);
      return Value<T>(node);
    }

    template<typename T> Value<T> mk_extvar(std::string name, void *base, T *data) {
      auto node(std::make_shared<fe::ExternalVariable>(this, name, fe::type_to_id(T()), base, (const char *)data - (const char *)base));
      extvars.emplace_back(node);
      return Value<T>(node);
    }

    template<typename T, int size> Array1D<T, size> mk_extvar(std::string name, void *base, T (*data)[size]) {
      auto node(std::make_shared<fe::ExternalVariable>(this, name, fe::type_to_id(T()), base, (const char *)data - (const char *)base));
      extvars.emplace_back(node);
      node->sizes.push_back(size);
      return Array1D<T, size>(node);
    }

    template<typename T, int size1, int size2> Array2D<T, size1, size2> mk_extvar(std::string name, void *base, T (*data)[size1][size2]) {
      auto node(std::make_shared<fe::ExternalVariable>(this, name, fe::type_to_id(T()), base, (const char *)data - (const char *)base));
      extvars.emplace_back(node);
      node->sizes.push_back(size1);
      node->sizes.push_back(size2);
      return Array2D<T, size1, size2>(node);
    }

    template<typename T> Value<T> mk_input(std::string name) {
      auto node(std::make_shared<fe::Input>(this, name, fe::POINTER));
      inputs.emplace_back(node);
      return Value<T>(node);
    }

    template<typename T> Value<T> mk_param(std::string name, const void *base, const T *data) {
      auto node(std::make_shared<fe::Parameter>(this, name, fe::type_to_id(T()), base, (const char *)data - (const char *)base));
      params.emplace_back(node);
      return Value<T>(node);
    }

    template<typename T, int size> Array1D<T, size> mk_param(std::string name, const void *base, T (*data)[size]) {
      auto node(std::make_shared<fe::Parameter>(this, name, fe::type_to_id(T()), base, (const char *)data - (const char *)base));
      node->sizes.push_back(size);
      params.emplace_back(node);
      return Array1D<T, size>(node);
    }

    template<typename T> Value<T> ext(T &data) {
      return Value<T>(std::make_shared<fe::UnOp>(this, fe::type_to_id(T()), fe::UNOP_DEREF, std::make_shared<fe::Constant>(this, fe::POINTER, &data)));
    }

    template<typename T, typename S = T, typename E = T> Value<T> range_loop(std::string name, Value<S> start, Value<E> end) {
      return Value<T>(this, 0);
    }

    template<typename T> void jif(Value<T> cond) {
      //..//
    }

    void jelse();
    void end();

  private:
    std::vector<std::shared_ptr<fe::Parameter>> params;
    std::vector<std::shared_ptr<fe::Variable>> vars;
    std::vector<std::shared_ptr<fe::ExternalVariable>> extvars;
    std::vector<std::shared_ptr<fe::Input>> inputs;
  };
}

#endif

#ifndef JIT_FE_H
#define JIT_FE_H

namespace jit {
  namespace fe {
    template<typename T> struct is_handled_integral_helper : public std::false_type {};
    template<> struct is_handled_integral_helper<std::int8_t  > : public std::true_type {};
    template<> struct is_handled_integral_helper<std::int16_t > : public std::true_type {};
    template<> struct is_handled_integral_helper<std::int32_t > : public std::true_type {};
    template<> struct is_handled_integral_helper<std::int64_t > : public std::true_type {};
    template<> struct is_handled_integral_helper<std::uint8_t > : public std::true_type {};
    template<> struct is_handled_integral_helper<std::uint16_t> : public std::true_type {};
    template<> struct is_handled_integral_helper<std::uint32_t> : public std::true_type {};
    template<> struct is_handled_integral_helper<std::uint64_t> : public std::true_type {};

    template<typename T> struct is_handled_integral : public is_handled_integral_helper<typename std::remove_cv<T>::type>::type {};

    template<typename A, typename B> struct binary_promotion {};
    template<> struct binary_promotion<std::int8_t  , std::int8_t  > { using type = std::int8_t;   };
    template<> struct binary_promotion<std::int8_t  , std::int16_t > { using type = std::int16_t;  };
    template<> struct binary_promotion<std::int8_t  , std::int32_t > { using type = std::int32_t;  };
    template<> struct binary_promotion<std::int8_t  , std::int64_t > { using type = std::int64_t;  };
    template<> struct binary_promotion<std::int16_t , std::int8_t  > { using type = std::int16_t;  };
    template<> struct binary_promotion<std::int16_t , std::int16_t > { using type = std::int16_t;  };
    template<> struct binary_promotion<std::int16_t , std::int32_t > { using type = std::int32_t;  };
    template<> struct binary_promotion<std::int16_t , std::int64_t > { using type = std::int64_t;  };
    template<> struct binary_promotion<std::int32_t , std::int8_t  > { using type = std::int32_t;  };
    template<> struct binary_promotion<std::int32_t , std::int16_t > { using type = std::int32_t;  };
    template<> struct binary_promotion<std::int32_t , std::int32_t > { using type = std::int32_t;  };
    template<> struct binary_promotion<std::int32_t , std::int64_t > { using type = std::int64_t;  };
    template<> struct binary_promotion<std::int64_t , std::int8_t  > { using type = std::int64_t;  };
    template<> struct binary_promotion<std::int64_t , std::int16_t > { using type = std::int64_t;  };
    template<> struct binary_promotion<std::int64_t , std::int32_t > { using type = std::int64_t;  };
    template<> struct binary_promotion<std::int64_t , std::int64_t > { using type = std::int64_t;  };
    template<> struct binary_promotion<std::int8_t  , std::uint8_t > { using type = std::uint8_t;  };
    template<> struct binary_promotion<std::int8_t  , std::uint16_t> { using type = std::uint16_t; };
    template<> struct binary_promotion<std::int8_t  , std::uint32_t> { using type = std::uint32_t; };
    template<> struct binary_promotion<std::int8_t  , std::uint64_t> { using type = std::uint64_t; };
    template<> struct binary_promotion<std::int16_t , std::uint8_t > { using type = std::uint16_t; };
    template<> struct binary_promotion<std::int16_t , std::uint16_t> { using type = std::uint16_t; };
    template<> struct binary_promotion<std::int16_t , std::uint32_t> { using type = std::uint32_t; };
    template<> struct binary_promotion<std::int16_t , std::uint64_t> { using type = std::uint64_t; };
    template<> struct binary_promotion<std::int32_t , std::uint8_t > { using type = std::uint32_t; };
    template<> struct binary_promotion<std::int32_t , std::uint16_t> { using type = std::uint32_t; };
    template<> struct binary_promotion<std::int32_t , std::uint32_t> { using type = std::uint32_t; };
    template<> struct binary_promotion<std::int32_t , std::uint64_t> { using type = std::uint64_t; };
    template<> struct binary_promotion<std::int64_t , std::uint8_t > { using type = std::uint64_t; };
    template<> struct binary_promotion<std::int64_t , std::uint16_t> { using type = std::uint64_t; };
    template<> struct binary_promotion<std::int64_t , std::uint32_t> { using type = std::uint64_t; };
    template<> struct binary_promotion<std::int64_t , std::uint64_t> { using type = std::uint64_t; };
    template<> struct binary_promotion<std::uint8_t , std::int8_t  > { using type = std::uint8_t;  };
    template<> struct binary_promotion<std::uint8_t , std::int16_t > { using type = std::uint16_t; };
    template<> struct binary_promotion<std::uint8_t , std::int32_t > { using type = std::uint32_t; };
    template<> struct binary_promotion<std::uint8_t , std::int64_t > { using type = std::uint64_t; };
    template<> struct binary_promotion<std::uint16_t, std::int8_t  > { using type = std::uint16_t; };
    template<> struct binary_promotion<std::uint16_t, std::int16_t > { using type = std::uint16_t; };
    template<> struct binary_promotion<std::uint16_t, std::int32_t > { using type = std::uint32_t; };
    template<> struct binary_promotion<std::uint16_t, std::int64_t > { using type = std::uint64_t; };
    template<> struct binary_promotion<std::uint32_t, std::int8_t  > { using type = std::uint32_t; };
    template<> struct binary_promotion<std::uint32_t, std::int16_t > { using type = std::uint32_t; };
    template<> struct binary_promotion<std::uint32_t, std::int32_t > { using type = std::uint32_t; };
    template<> struct binary_promotion<std::uint32_t, std::int64_t > { using type = std::uint64_t; };
    template<> struct binary_promotion<std::uint64_t, std::int8_t  > { using type = std::uint64_t; };
    template<> struct binary_promotion<std::uint64_t, std::int16_t > { using type = std::uint64_t; };
    template<> struct binary_promotion<std::uint64_t, std::int32_t > { using type = std::uint64_t; };
    template<> struct binary_promotion<std::uint64_t, std::int64_t > { using type = std::uint64_t; };
    template<> struct binary_promotion<std::uint8_t , std::uint8_t > { using type = std::uint8_t;  };
    template<> struct binary_promotion<std::uint8_t , std::uint16_t> { using type = std::uint16_t; };
    template<> struct binary_promotion<std::uint8_t , std::uint32_t> { using type = std::uint32_t; };
    template<> struct binary_promotion<std::uint8_t , std::uint64_t> { using type = std::uint64_t; };
    template<> struct binary_promotion<std::uint16_t, std::uint8_t > { using type = std::uint16_t; };
    template<> struct binary_promotion<std::uint16_t, std::uint16_t> { using type = std::uint16_t; };
    template<> struct binary_promotion<std::uint16_t, std::uint32_t> { using type = std::uint32_t; };
    template<> struct binary_promotion<std::uint16_t, std::uint64_t> { using type = std::uint64_t; };
    template<> struct binary_promotion<std::uint32_t, std::uint8_t > { using type = std::uint32_t; };
    template<> struct binary_promotion<std::uint32_t, std::uint16_t> { using type = std::uint32_t; };
    template<> struct binary_promotion<std::uint32_t, std::uint32_t> { using type = std::uint32_t; };
    template<> struct binary_promotion<std::uint32_t, std::uint64_t> { using type = std::uint64_t; };
    template<> struct binary_promotion<std::uint64_t, std::uint8_t > { using type = std::uint64_t; };
    template<> struct binary_promotion<std::uint64_t, std::uint16_t> { using type = std::uint64_t; };
    template<> struct binary_promotion<std::uint64_t, std::uint32_t> { using type = std::uint64_t; };
    template<> struct binary_promotion<std::uint64_t, std::uint64_t> { using type = std::uint64_t; };

    enum type_t {
      CONSTANT,
      UNOP,
      BIOP,
      TERNARY,
      CONVERT,
      PARAMETER,
      VARIABLE,
      EXTVAR
    };

    enum type_id_t {
      BOOL,
      S8, S16, S32, S64,
      U8, U16, U32, U64,
      POINTER
    };

    enum unop_id_t {
      UNOP_DEREF,
      UNOP_NEG,
      UNOP_NOT
    };

    enum biop_id_t {
      BIOP_BAND,
      BIOP_BOR,
      BIOP_BXOR,
      BIOP_ADD,
      BIOP_SUB,
      BIOP_MUL,
      BIOP_DIV,
      BIOP_LAND,
      BIOP_LOR,
      BIOP_LE,
      BIOP_LT,
      BIOP_GE,
      BIOP_GT,
      BIOP_EQ,
      BIOP_NE,
      BIOP_LSHIFT,
      BIOP_RSHIFT
    };

    constexpr type_id_t type_to_id(bool)          { return BOOL; };
    constexpr type_id_t type_to_id(std::int8_t  ) { return S8;  };
    constexpr type_id_t type_to_id(std::int16_t ) { return S16; };
    constexpr type_id_t type_to_id(std::int32_t ) { return S32; };
    constexpr type_id_t type_to_id(std::int64_t ) { return S64; };
    constexpr type_id_t type_to_id(std::uint8_t ) { return U8;  };
    constexpr type_id_t type_to_id(std::uint16_t) { return U16; };
    constexpr type_id_t type_to_id(std::uint32_t) { return U32; };
    constexpr type_id_t type_to_id(std::uint64_t) { return U64; };
    template<typename U> constexpr type_id_t type_to_id(U *) { return POINTER; };

    class Generator {
    public:
      Generator(Context &context);
      unsigned int current_block();

    protected:
      Context &context;
    };

    class Node {
    public:
      Generator *gen;
      unsigned int block;
      type_t type;
      type_id_t ntype;

      Node(type_t _type, type_id_t _ntype, Generator *_gen) : type(_type), ntype(_ntype), gen(_gen), block(_gen->current_block()) { }
      virtual ~Node() = default;
      
      Node(Node const&) = delete;
      Node(Node&&) = delete;
      Node& operator=(Node const&) = delete;
      Node& operator=(Node&&) = delete;
    };

    class Constant : public Node {
    public:
      std::uint64_t value;

      Constant(Generator *_gen, type_id_t _ntype, std::uint64_t _value) : Node(CONSTANT, _ntype, _gen), value(_value) {}
      template<typename T> Constant(Generator *_gen, type_id_t _ntype, T *_value) : Node(CONSTANT, _ntype, _gen), value(reinterpret_cast<std::uint64_t>(_value)) {}
    };

    class Convert : public Node {
    public:
      std::shared_ptr<Node> a;
      type_id_t itype;

      Convert(Generator *_gen, type_id_t _ntype, type_id_t _itype, std::shared_ptr<Node> _a) :
	Node(CONVERT, _ntype, _gen), itype(_itype), a(std::move(_a)){}
    };

    class UnOp : public Node {
    public:
      std::shared_ptr<Node> a;
      unop_id_t op;

      UnOp(Generator *_gen, type_id_t _ntype, unop_id_t _op, std::shared_ptr<Node> _a) :
	Node(UNOP, _ntype, _gen), op(_op), a(std::move(_a)) {}
    };

    class BiOp : public Node {
    public:
      std::shared_ptr<Node> a, b;
      biop_id_t op;

      BiOp(Generator *_gen, type_id_t _ntype, biop_id_t _op, std::shared_ptr<Node> _a, std::shared_ptr<Node> _b) :
	Node(BIOP, _ntype, _gen), op(_op), a(std::move(_a)), b(std::move(_b)) {}
    };

    class Ternary : public Node {
    public:
      std::shared_ptr<Node> test, yes, no;
      type_id_t otype;

      Ternary(Generator *_gen, type_id_t _ntype, std::shared_ptr<Node> _test, std::shared_ptr<Node> _yes, std::shared_ptr<Node> _no) :
	Node(TERNARY, _ntype, _gen), test(std::move(_test)), yes(std::move(_yes)), no(std::move(_no)) {}
    };

    class Parameter : public Node {
    public:
      std::string name;
      const void *base;
      std::vector<size_t> sizes;
      std::ptrdiff_t offset;
      type_id_t otype;

      Parameter(Generator *_gen, std::string _name, type_id_t _ntype, const void *_base, std::ptrdiff_t _offset) :
	Node(PARAMETER, _ntype, _gen), name(_name), base(_base), offset(_offset) {}
    };

    class Variable : public Node {
    public:
      std::string name;
      std::shared_ptr<Node> init;
      type_id_t otype;

      Variable(Generator *_gen, std::string _name, type_id_t _ntype, std::shared_ptr<Node> _init) :
	Node(VARIABLE, _ntype, _gen), name(_name), init(std::move(_init)) {}
    };

    class ExternalVariable : public Node {
    public:
      std::string name;
      const void *base;
      std::vector<size_t> sizes;
      std::ptrdiff_t offset;
      type_id_t otype;

      ExternalVariable(Generator *_gen, std::string _name, type_id_t _ntype, const void *_base, std::ptrdiff_t _offset) :
	Node(EXTVAR, _ntype, _gen), name(_name), base(_base), offset(_offset) {}
    };

    class Input : public Node {
    public:
      std::string name;

      Input(Generator *_gen, std::string _name, type_id_t _ntype) :
	Node(VARIABLE, _ntype, _gen), name(_name) {}
    };

  }
}

#endif

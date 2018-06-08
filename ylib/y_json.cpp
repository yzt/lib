#include "y_json.h"

#include <cassert>

#define Y_OPT_JSON_TRACK_LINE_AND_COLUMN        1
#define Y_OPT_JSON_CALC_SUBSTR_UNESCAPED_SIZE   1

#define Y_ASSERT(cond, ...)         assert(cond)
#define Y_ASSERT_STRONG(cond, ...)  assert(cond)

struct Exception {
    json_error_severity_e severity;
    json_error_e error;
    json_location_t location;
    char const * msg;
    int param1;
    int param2;
};

struct State {
    json_buffer_t in;
    int cur;    // char, or -1 if EOI; Note: this must be initialized at the beginning.

    json_location_t loc;

    json_user_handle_t parent;
    json_substr_t cur_name;

    json_element_f elem_cb;
    json_error_f error_cb;
    void * user_data;
};

//static inline json_buffer_t
//GetInput (State * state) {
//    return {
//        state->ptr,
//        state->size,
//    };
//}

//static inline json_location_t
//GetLocation (State * state) {
//    return {
//        state->depth,
//        state->pos,
//        state->line,
//        state->column,
//    };
//}

static inline bool
eoi (State const * state) {
    return state->loc.byte >= state->in.size;
}

static inline void
UpdateCurChar (State * state) {
    if (!eoi(state))
        state->cur = state->in.ptr[state->loc.byte];
    else
        state->cur = -1;
}

static inline void
Advance (State * state) {
#if defined(Y_OPT_JSON_TRACK_LINE_AND_COLUMN)
    if ('\n' == state->in.ptr[state->loc.byte]) {
        state->loc.line += 1;
        state->loc.column = 1;
    } else
        state->loc.column += 1;
#endif
    state->loc.byte += 1;

    UpdateCurChar(state);    
}

// Note: this is too simplistic. There are UNICODE whitespace characters not considered here.
static inline bool
is_whitespace (int c) {
    return ' ' == c || '\t' == c || '\n' == c || '\r' == c /*|| '\f' == c || '\v' == c || '\b' == c*/;
}

static inline void
consume (State * state) {
    if (eoi(state)) {
        throw Exception{JSON_SEV_Error, JSON_ERR_IncompleteInput, state->loc, "This should not have happened!", 0, 0};
    }
    Advance(state);
}

static inline void
expect (State * state, char expected) {    // also consumes the character
    if (state->cur != expected) {
        throw Exception{JSON_SEV_Error, JSON_ERR_ExpectedToken, state->loc, "Expected A, but found X", expected, state->cur};
    }
    Advance(state);
}

static inline void
expect (State * state, char expected1, char expected2) {
    if (state->cur != expected1 && state->cur != expected2) {
        throw Exception{JSON_SEV_Error, JSON_ERR_ExpectedToken, state->loc, "Expected A or B, but found X", expected1, expected2};
    }
    Advance(state);
}

static inline void
skip_ws (State * state) {
    while (!eoi(state) && is_whitespace(state->cur))
        Advance(state);
}

//static inline char
//skip_ws_and_peek (State * state) {
//
//}

//static inline void
//skip_ws_and_expect (State * state, char expected) {
//    skip_ws(state);
//    expect(state, expected);
//}

//static inline void
//skip_ws_and_expect (State * state, char expected1, char expected2) {
//    skip_ws(state);
//    expect(state, expected1, expected2);
//}

//static inline char
//consume_and_skip_ws (State * state) {
//    consume(state);
//    skip_ws(state);
//}

static inline json_substr_t
read_str (State * state) {
    expect(state, '"');
    auto begin = state->loc.byte;
    int unescaped = 0;
    bool prev_was_backslash = false;
    while (!eoi(state)) {
        if (!prev_was_backslash) {
            if ('"' == state->cur)
                break;
            if ('\\' == state->cur)
                prev_was_backslash = true;
        } else {    // previous character was actually a backslash
            switch (state->cur) {
            case 'u': unescaped -= 5; break;
            case '"': case '\\': case '/': case 'b': case 'f': case 'n': case 'r': case 't': unescaped -= 1; break;
            default: throw Exception{JSON_SEV_Pedantic, JSON_ERR_BadEscaping, state->loc, "Unknown string escape sequence", state->cur, 0}; break;
            }
            prev_was_backslash = false;
        }
        Advance(state);
        unescaped += 1;
    }
    auto end = state->loc.byte;
    expect(state, '"');

    return {begin, end, json_size_t(unescaped)};
}


static void
parse_value (State * state);

static void
parse_object (State * state) {
    //skip_ws(state);
    expect(state, '{');

    auto old_parent = state->parent;
    auto self = state->elem_cb(JSON_ETYPE_ObjectBegin, nullptr, {}, state->parent, &state->in, &state->loc, state->user_data);
    state->parent = self;
    state->loc.depth += 1;

    skip_ws(state);
    while ('}' != state->cur) {
        state->cur_name = read_str(state);
        skip_ws(state);
        expect(state, ':');
        skip_ws(state);
        parse_value(state);
        skip_ws(state);
        if (',' != state->cur && '}' != state->cur)
            throw Exception{JSON_SEV_Error, JSON_ERR_ExpectedToken, state->loc, "Expected ',' or '}' in object", 0, 0};
        if (',' == state->cur) {
            consume(state);
            skip_ws(state);
        }
    }
    consume(state);

    state->loc.depth -= 1;
    state->parent = old_parent;
    state->elem_cb(JSON_ETYPE_ObjectEnd, self, {}, old_parent, &state->in, &state->loc, state->user_data);
}

static void
parse_array (State * state) {
    expect(state, '[');

    auto old_parent = state->parent;
    auto self = state->elem_cb(JSON_ETYPE_ArrayBegin, nullptr, {}, state->parent, &state->in, &state->loc, state->user_data);
    state->parent = self;
    state->loc.depth += 1;

    skip_ws(state);
    while (']' != state->cur) {
        skip_ws(state);
        parse_value(state);
        skip_ws(state);
        if (',' != state->cur && ']' != state->cur)
            throw Exception{JSON_SEV_Error, JSON_ERR_ExpectedToken, state->loc, "Expected ',' or ']' in array", 0, 0};
        if (',' == state->cur)
            consume(state);
    }
    consume(state);

    state->loc.depth -= 1;
    state->parent = old_parent;
    state->elem_cb(JSON_ETYPE_ArrayEnd, self, {}, old_parent, &state->in, &state->loc, state->user_data);
}

static void
parse_string (State * state) {
}

static void
parse_bool (State * state) {
}

static void
parse_null (State * state) {
}

static void
parse_number (State * state) {
}

static void
parse_value (State * state) {
    switch (state->cur) {
    case '{': parse_object(state); break;
    case '[': parse_array(state); break;
    case '"': parse_string(state); break;
    case 'n': parse_null(state); break;
    case 't': 
    case 'f': parse_bool(state); break;
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
    case '-':
    case '+':
    case '.': parse_number(state); break;
    default: throw Exception{JSON_SEV_Error, JSON_ERR_ExpectedValue, state->loc, "a JSON value should start with one of these characters: 0123456789+-.{[\"ntf", state->cur, 0}; break;
    }
}

#if 0
static inline bool
LocAdvance (
    json_buffer_t in,
    json_location_t & loc
) {
    bool ret = false;
    if (!loc.eoi && loc.byte < in.size) {
        if ('\n' == in.ptr[loc.byte]) {
            loc.line += 1;
            loc.column = 1;
        } else {
            loc.column += 1;
        }
        loc.byte += 1;
        if (loc.byte < in.size)
            ret = true;
        else
            loc.eoi = true;
    }
    return ret;
}

// Note: this is too simplistic. There are UNICODE whitespace characters not considered here.
static inline bool
IsWhitespace (
    char c
) {
    return ' ' == c || '\t' == c || '\n' == c || '\r' == c || '\f' == c || '\v' == c;
}

static inline bool
SkipWhitespace (
    json_buffer_t const & in,
    json_location_t & loc
) {
    bool ret = false;
    ret = (!loc.eoi && !loc.has_error);
    while (!loc.eoi && IsWhitespace(in.ptr[loc.byte]))
        LocAdvance(in, loc);
    return ret;
}

static bool
ParseObject (
    json_buffer_t const & in,
    json_location_t & loc,
    json_user_handle_t parent,
    json_element_f elem_cb,
    json_error_f error_cb,
    void * user_data
) {
    char c = '\0';

    // skip WS and peek
    SkipWhitespace(in, loc);
    if (loc.eoi || loc.has_error)
        return false;
    c = in.ptr[loc.byte];

    // check expectation
    if ('{' != c) {
        error_cb(
            JSON_SEV_Error, JSON_ERR_ExpectedToken, &in, &loc, user_data,
            "Expected a '{' for an object", '{'
        );
        return false;
    }

    // callback the user
    auto obj_handle = elem_cb(
        JSON_ETYPE_ObjectBegin, JSON_VTYPE_INVALID, {},
        parent, &in, &loc, user_data
    );

    // increase hierarchy depth
    loc.depth += 1;

    // consume and skip WS and peek
    LocAdvance(in, loc);
    SkipWhitespace(in, loc);
    if (loc.eoi || loc.has_error) {
        error_cb(JSON_SEV_Fatal, JSON_ERR_IncompleteInput, &in, &loc, user_data, "Found a '{' but nothing after", 0);
        return false;
    }
    c = in.ptr[loc.byte];

    bool done = false;
    int child_count = 0;

    char c = in.ptr[loc.byte];
    while ('}' != c) {
        
    }
    
    Y_ASSERT('}' == c);
    elem_cb(...);

    return done;
}
#endif

json_size_t
JSON_Parse (
    json_buffer_t in,
    json_element_f elem_cb,
    json_error_f error_cb,
    void * user_data
) {
    json_location_t loc {0, 0, 1, 0};
    if (!error_cb) {
        return 0;
    }
    if (!elem_cb) {
        error_cb(JSON_SEV_Fatal, JSON_ERR_BadParams, &in, &loc, user_data, "The element handling callback must be valid", 0, 0);
        return 0;
    }
    if (!elem_cb || (!in.ptr && in.size > 0)) {
        error_cb(JSON_SEV_Fatal, JSON_ERR_BadParams, &in, &loc, user_data, "Invalid input (no pointer with size > 0)", 0, 0); 
        return 0;
    }

    State state;
    state.in = in;
    state.cur = in.ptr[0];
    state.loc = loc;
    state.parent = nullptr;
    state.cur_name = {};
    state.elem_cb = elem_cb;
    state.error_cb = error_cb;
    state.user_data = user_data;

    try {
        skip_ws(&state);
        parse_object(&state);
    } catch (Exception & e) {
        state.error_cb(e.severity, e.error, &state.in, &e.location, state.user_data, e.msg, e.param1, e.param2);
    }

    return state.loc.byte;
}


#if 0
#pragma once

#if !defined(__KAGE__JSON_H__)
	#define  __KAGE__JSON_H__

//======================================================================

#include <kage/config.h>
#include <kage/common.h>
#include <kage/stl/string.h>
#include <kage/stl/stringstream.h>
#include <kage/stl/vector.h>

#include <cmath>	// pow()
#include <cstdint>	// int64_t
#include <cstdio>	// snprintf()

//======================================================================

#if defined(_MSC_VER)
	#define snprintf _snprintf
#endif

//======================================================================

namespace Kage {
	namespace JSON {

//======================================================================

class Value;
class Object;

//----------------------------------------------------------------------

typedef bool Bool;
typedef int64_t Int;
typedef double Real;
typedef STL::String String;
typedef STL::Vector<Value> Array;

//----------------------------------------------------------------------

enum class Type : uint8_t
{
	Null,
	Bool,
	Int,
	Real,
	String,
	Array,
	Object,
};

//----------------------------------------------------------------------

enum class PrintType
{
	Compact = 1,
	Smarter = 2,	// Object fields on new lines, arrays on the same line.
	Pretty = 3,		// Newlines and tabs for everything
};

//======================================================================

template <typename Archive>
Value Parse (Archive & arch);

//======================================================================

class Value
{
public:
	Value ();
	explicit Value (Type type);
	Value (Value const & that);
	Value (Value && that);
	Value (Bool b);
	Value (Int i);
	Value (int i);
	Value (Real r);
	Value (String s);
	Value (char const * s);
	Value (char const * s_begin, char const * s_end);
	Value (Array a);
	Value (Object o);
	~Value ();

	Value & operator = (Value const & that);
	Value & operator = (Value && that);
	Value & operator = (Bool b);
	Value & operator = (Int i);
	Value & operator = (int i);
	Value & operator = (Real r);
	Value & operator = (String const & s);
	Value & operator = (String && s);
	Value & operator = (char const * s);
	Value & operator = (Array const & a);
	Value & operator = (Array && a);
	Value & operator = (Object const & o);
	Value & operator = (Object && o);

	Type type () const;
	bool typeIs (Type type) const;
	void setType (Type new_type);

	bool isNull () const;
	bool isBool () const;
	bool isInt () const;
	bool isReal () const;
	bool isStr () const;
	bool isArray () const;
	bool isObject () const;

	//template <Type t> auto get () const -> typename _TypeMap<t>::const_ref_type;
	//template <Type t> auto get () -> typename _TypeMap<t>::ref_type;

	Bool asBool () const;
	Bool & asBool ();
	Int asInt () const;
	Int & asInt ();
	Real asReal () const;
	Real & asReal ();
	String const & asStr () const;
	String & asStr ();
	Array const & asArray () const;
	Array & asArray ();
	Object const & asObj () const;
	Object & asObj ();

	template <typename Archive>
	void write (Archive & arch, PrintType ptype, unsigned indent = 0) const;

private:
	void constructDefault ();
	void constructCopy (Value const & that);
	void constructMove (Value && that);
	void destruct ();

private:
	union {
		Bool b;
		Int i;
		Real r;
		String * s;
		Array * a;
		Object * o;
	} m_data;
	Type m_type;

public:
	template <typename Archive>	static void WriteString (Archive & arch, String const & str);
	template <typename Archive>	static void WriteReal (Archive & arch, Real r);

	template <typename Archive> static Value ReadNull (Archive & arch);
	template <typename Archive>	static Value ReadBool (Archive & arch);
	template <typename Archive>	static Value ReadInt (Archive & arch);
	template <typename Archive>	static Value ReadReal (Archive & arch);
	template <typename Archive>	static Value ReadNumber (Archive & arch);
	template <typename Archive>	static Value ReadString (Archive & arch);
	template <typename Archive>	static Value ReadArray (Archive & arch);
	template <typename Archive>	static Value ReadObject (Archive & arch);
};

//======================================================================

class Object
{
public:
	typedef std::pair<String, Value> Field;

public:
	template <typename Archive>
	static Object Read (Archive & arch);

public:
	Object ();
	Object (Object const & that);
	Object (Object && that);
	~Object ();

	Object & operator = (Object const & that);
	Object & operator = (Object && that);

	size_t size () const;
	bool empty () const;
	Field & at (size_t indx);
	Field const & at (size_t indx) const;

	// Iff not found, returns >= size
	size_t findIndex (String const & name) const;
	bool has (String const & name) const;

	// If doesn't exist, the result is undefined
	Value const & find (String const & name, Value const & default_ret) const;
	Value & find (String const & name, Value & default_ret);

	Value & add (Field f);
	Value & add (String s, Value v);

	void clear ();

	template <typename Archive>
	void write (Archive & arch, PrintType ptype, unsigned indent = 0) const;

	String toStringSlow (PrintType ptype = PrintType::Compact, unsigned indent = 0) const;

private:
	STL::Vector<Field> m_fields;
};

//======================================================================

	}	// namespace JSON
}	// namespace Kage

//======================================================================
// Implementation:
//======================================================================

namespace Kage {
	namespace JSON {

//======================================================================
//----------------------------------------------------------------------

inline Value::Value ()
	: m_type (Type::Null)
{
}

//----------------------------------------------------------------------

inline Value::Value (Type type)
	: m_type (type)
{
	constructDefault ();
}

//----------------------------------------------------------------------

inline Value::Value (Value const & that)
{
	constructCopy (that);
}

//----------------------------------------------------------------------

inline Value::Value (Value && that)
{
	constructMove (std::forward<Value&&>(that));
}

//----------------------------------------------------------------------

inline Value::Value (Bool b)
	: m_type (Type::Bool)
{
	m_data.b = b;
}

//----------------------------------------------------------------------

inline Value::Value (Int i)
	: m_type (Type::Int)
{
	m_data.i = i;
}

//----------------------------------------------------------------------

inline Value::Value (int i)
	: m_type (Type::Int)
{
	m_data.i = i;
}

//----------------------------------------------------------------------

inline Value::Value (Real r)
	: m_type (Type::Real)
{
	m_data.r = r;
}

//----------------------------------------------------------------------

inline Value::Value (String s)
	: m_type (Type::String)
{
	m_data.s = KAGE_NEW String (std::move(s));
}

//----------------------------------------------------------------------

inline Value::Value (char const * s)
	: m_type (Type::String)
{
	m_data.s = KAGE_NEW String (s);
}

//----------------------------------------------------------------------

inline Value::Value (char const * s_begin, char const * s_end)
	: m_type (Type::String)
{
	m_data.s = KAGE_NEW String (s_begin, s_end);
}

//----------------------------------------------------------------------

inline Value::Value (Array a)
	: m_type (Type::Array)
{
	m_data.a = KAGE_NEW Array (std::move(a));
}

//----------------------------------------------------------------------

inline Value::Value (Object o)
	: m_type (Type::Object)
{
	m_data.o = KAGE_NEW Object (std::move(o));
}

//----------------------------------------------------------------------

inline Value::~Value ()
{
	destruct ();
}

//----------------------------------------------------------------------

inline Value & Value::operator = (Value const & that)
{
	if (&that != this)
	{
		destruct ();
		constructCopy (that);
	}

	return *this;
}

//----------------------------------------------------------------------

inline Value & Value::operator = (Value && that)
{
	if (&that != this)
	{
		destruct ();
		constructMove (std::forward<Value &&>(that));
	}

	return *this;
}

//----------------------------------------------------------------------

inline Value & Value::operator = (Bool b)
{
	destruct ();
	m_type = Type::Bool;
	m_data.b = b;

	return *this;
}

//----------------------------------------------------------------------

inline Value & Value::operator = (Int i)
{
	destruct ();
	m_type = Type::Int;
	m_data.i = i;

	return *this;
}

//----------------------------------------------------------------------

inline Value & Value::operator = (int i)
{
	destruct ();
	m_type = Type::Int;
	m_data.i = i;

	return *this;
}

//----------------------------------------------------------------------

inline Value & Value::operator = (Real r)
{
	destruct ();
	m_type = Type::Real;
	m_data.r = r;

	return *this;
}

//----------------------------------------------------------------------

inline Value & Value::operator = (String const & s)
{
	destruct ();
	m_type = Type::String;
	m_data.s = KAGE_NEW String (s);

	return *this;
}

//----------------------------------------------------------------------

inline Value & Value::operator = (String && s)
{
	destruct ();
	m_type = Type::String;
	m_data.s = KAGE_NEW String (std::move(s));

	return *this;
}

//----------------------------------------------------------------------

inline Value & Value::operator = (char const * s)
{
	destruct ();
	m_type = Type::String;
	m_data.s = KAGE_NEW String (s);

	return *this;
}

//----------------------------------------------------------------------

inline Value & Value::operator = (Array const & a)
{
	destruct ();
	m_type = Type::Array;
	m_data.a = KAGE_NEW Array (a);

	return *this;
}

//----------------------------------------------------------------------

inline Value & Value::operator = (Array && a)
{
	destruct ();
	m_type = Type::Array;
	m_data.a = KAGE_NEW Array (std::move(a));

	return *this;
}

//----------------------------------------------------------------------

inline Value & Value::operator = (Object const & o)
{
	destruct ();
	m_type = Type::Object;
	m_data.o = KAGE_NEW Object (o);

	return *this;
}

//----------------------------------------------------------------------

inline Value & Value::operator = (Object && o)
{
	destruct ();
	m_type = Type::Object;
	m_data.o = KAGE_NEW Object (std::move(o));

	return *this;
}

//----------------------------------------------------------------------

inline Type Value::type () const
{
	return m_type;
}

//----------------------------------------------------------------------

inline bool Value::typeIs (Type type) const
{
	return m_type == type;
}

//----------------------------------------------------------------------

inline void Value::setType (Type new_type)
{
	if (new_type != m_type)
	{
		destruct ();
		m_type = new_type;
		constructDefault ();
	}
}

//----------------------------------------------------------------------

inline bool Value::isNull () const
{
	return m_type == Type::Null;
}

//----------------------------------------------------------------------

inline bool Value::isBool () const
{
	return m_type == Type::Bool;
}

//----------------------------------------------------------------------

inline bool Value::isInt () const
{
	return m_type == Type::Int;
}

//----------------------------------------------------------------------

inline bool Value::isReal () const
{
	return m_type == Type::Real;
}

//----------------------------------------------------------------------

inline bool Value::isStr () const
{
	return m_type == Type::String;
}

//----------------------------------------------------------------------

inline bool Value::isArray () const
{
	return m_type == Type::Array;
}

//----------------------------------------------------------------------

inline bool Value::isObject () const
{
	return m_type == Type::Object;
}

//----------------------------------------------------------------------

inline Bool Value::asBool () const
{
	KAGE_ASSERT_STRONG (isBool());
	return m_data.b;
}

//----------------------------------------------------------------------

inline Bool & Value::asBool ()
{
	KAGE_ASSERT_STRONG (isBool());
	return m_data.b;
}

//----------------------------------------------------------------------

inline Int Value::asInt () const
{
	KAGE_ASSERT_STRONG (isInt());
	return m_data.i;
}

//----------------------------------------------------------------------

inline Int & Value::asInt ()
{
	KAGE_ASSERT_STRONG (isInt());
	return m_data.i;
}

//----------------------------------------------------------------------

inline Real Value::asReal () const
{
	KAGE_ASSERT_STRONG (isReal());
	return m_data.r;
}

//----------------------------------------------------------------------

inline Real & Value::asReal ()
{
	KAGE_ASSERT_STRONG (isReal());
	return m_data.r;
}

//----------------------------------------------------------------------

inline String const & Value::asStr () const
{
	KAGE_ASSERT_STRONG (isStr());
	return *m_data.s;
}

//----------------------------------------------------------------------

inline String & Value::asStr ()
{
	KAGE_ASSERT_STRONG (isStr());
	return *m_data.s;
}

//----------------------------------------------------------------------

inline Array const & Value::asArray () const
{
	KAGE_ASSERT_STRONG (isArray());
	return *m_data.a;
}

//----------------------------------------------------------------------

inline Array & Value::asArray ()
{
	KAGE_ASSERT_STRONG (isArray());
	return *m_data.a;
}

//----------------------------------------------------------------------

inline Object const & Value::asObj () const
{
	KAGE_ASSERT_STRONG (isObject());
	return *m_data.o;
}

//----------------------------------------------------------------------

inline Object & Value::asObj ()
{
	KAGE_ASSERT_STRONG (isObject());
	return *m_data.o;
}

//----------------------------------------------------------------------

template <typename Archive>
inline void Value::write (Archive & arch, PrintType ptype, unsigned indent /*= 0*/) const
{
	switch (m_type)
	{
	case Type::Null:
		arch << "null";
		break;

	case Type::Bool:
		arch << (m_data.b ? "true" : "false");
		break;

	case Type::Int:
		arch << m_data.i;
		break;

	case Type::Real:
		WriteReal (arch, m_data.r);
		break;

	case Type::String:
		WriteString (arch, *m_data.s);
		break;

	case Type::Array:
		arch << '[';
		for (size_t i = 0, e = m_data.a->size(); i < e; ++i)
		{
			if (i > 0) arch << ',';
			if (ptype == PrintType::Pretty) arch << '\n' << String(indent + 1, '\t');
			(*m_data.a)[i].write (arch, ptype, indent + 1);
		}
		if (ptype == PrintType::Pretty) arch << '\n' << String(indent, '\t');
		arch << ']';
		break;

	case Type::Object:
		m_data.o->write (arch, ptype, indent);
		break;
	}
}

//----------------------------------------------------------------------
//----------------------------------------------------------------------

// m_type must be already set
inline void Value::constructDefault ()
{
	switch (m_type)
	{
		case Type::Bool:   m_data.b = false; break;
		case Type::Int:    m_data.i = 0; break;
		case Type::Real:   m_data.r = 0; break;
		case Type::String: m_data.s = KAGE_NEW String; break;
		case Type::Array:  m_data.a = KAGE_NEW Array; break;
		case Type::Object: m_data.o = KAGE_NEW Object; break;
		default: break;
	}
}

//----------------------------------------------------------------------

inline void Value::constructCopy (Value const & that)
{
	m_type = that.m_type;

	switch (m_type)
	{
		case Type::Bool:   m_data.b = that.m_data.b; break;
		case Type::Int:    m_data.i = that.m_data.i; break;
		case Type::Real:   m_data.r = that.m_data.r; break;
		case Type::String: m_data.s = KAGE_NEW String (*that.m_data.s); break;
		case Type::Array:  m_data.a = KAGE_NEW Array (*that.m_data.a); break;
		case Type::Object: m_data.o = KAGE_NEW Object (*that.m_data.o); break;
		default: break;
	}
}

//----------------------------------------------------------------------

inline void Value::constructMove (Value && that)
{
	m_type = that.m_type;

	switch (m_type)
	{
		case Type::Bool:   m_data.b = that.m_data.b; break;
		case Type::Int:    m_data.i = that.m_data.i; break;
		case Type::Real:   m_data.r = that.m_data.r; break;
		case Type::String: m_data.s = KAGE_NEW String (std::move(*that.m_data.s)); break;
		case Type::Array:  m_data.a = KAGE_NEW Array (std::move(*that.m_data.a)); break;
		case Type::Object: m_data.o = KAGE_NEW Object (std::move(*that.m_data.o)); break;
		default: break;
	}

	that.destruct ();
}

//----------------------------------------------------------------------

inline void Value::destruct ()
{
	switch (m_type)
	{
		case Type::String: KAGE_DELETE (m_data.s); break;
		case Type::Array:  KAGE_DELETE (m_data.a); break;
		case Type::Object: KAGE_DELETE (m_data.o); break;
		default: break;
	}

	m_type = Type::Null;
}

//----------------------------------------------------------------------
//======================================================================

inline Object::Object ()
	: m_fields ()
{
}

//----------------------------------------------------------------------

inline Object::Object (Object const & that)
	: m_fields (that.m_fields)
{
}

//----------------------------------------------------------------------

inline Object::Object (Object && that)
	: m_fields (std::move(that.m_fields))
{
}

//----------------------------------------------------------------------

inline Object::~Object ()
{
}

//----------------------------------------------------------------------

inline Object & Object::operator = (Object const & that)
{
	if (&that != this)
		m_fields = that.m_fields;

	return *this;
}

//----------------------------------------------------------------------

inline Object & Object::operator = (Object && that)
{
	if (&that != this)
		m_fields = std::move(that.m_fields);

	return *this;
}

//----------------------------------------------------------------------

inline size_t Object::size () const
{
	return m_fields.size();
}

//----------------------------------------------------------------------

inline bool Object::empty () const
{
	return m_fields.empty();
}

//----------------------------------------------------------------------

inline Object::Field & Object::at (size_t indx)
{
	return m_fields[indx];
}

//----------------------------------------------------------------------

inline Object::Field const & Object::at (size_t indx) const
{
	return m_fields[indx];
}

//----------------------------------------------------------------------

inline size_t Object::findIndex (String const & name) const
{
	for (size_t i = 0; i < m_fields.size(); ++i)
		if (name == m_fields[i].first)
			return i;
	return m_fields.size();
}

//----------------------------------------------------------------------

inline bool Object::has (String const & name) const
{
	return findIndex(name) < size();
}

//----------------------------------------------------------------------

inline Value const & Object::find (String const & name, Value const & default_ret) const
{
	auto i = findIndex(name);
	if (i < size())
		return m_fields[i].second;
	else
		return default_ret;
}

//----------------------------------------------------------------------

inline Value & Object::find (String const & name, Value & default_ret)
{
	auto i = findIndex(name);
	if (i < size())
		return m_fields[i].second;
	else
		return default_ret;
}

//----------------------------------------------------------------------

inline Value & Object::add (Field f)
{
	m_fields.push_back (std::move(f));
	return m_fields.back().second;
}

//----------------------------------------------------------------------

inline Value & Object::add (String s, Value v)
{
	m_fields.emplace_back (std::move(s), std::move(v));
	return m_fields.back().second;
}

//----------------------------------------------------------------------

inline void Object::clear ()
{
	m_fields.clear ();
}

//----------------------------------------------------------------------

template <typename Archive>
inline void Object::write (Archive & arch, PrintType ptype, unsigned indent /*= 0*/) const
{
	bool pretty = (ptype != PrintType::Compact);
	arch << '{';
	for (size_t i = 0, e = m_fields.size(); i < e; ++i)
	{
		if (i > 0) arch << ',';
		if (pretty) arch << '\n' << String(indent + 1, '\t');
		Value::WriteString (arch, m_fields[i].first);
		arch << (pretty ? ": " : ":");
		m_fields[i].second.write (arch, ptype, indent + 1);
	}
	if (pretty) arch << '\n' << String(indent, '\t');
	arch << '}';
}

//----------------------------------------------------------------------

inline String Object::toStringSlow (PrintType ptype, unsigned indent) const
{
	STL::StringStream ss;
	write (ss, ptype, indent);
	return ss.str();
}

//======================================================================

namespace _parser_details {
	template <typename T>
	bool _is_ws (T const c)
	{
		return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f';
	}

	template <typename T>
	bool _is_digit (T const c)
	{
		return '0' <= c && c <= '9';
	}

	template <typename Archive>
	typename Archive::int_type _peek (Archive & arch)
	{
		return arch.peek ();
	}

	template <typename Archive>
	typename Archive::int_type _consume_and_peek (Archive & arch)
	{
		typename Archive::char_type dummy;
		arch.get (dummy);
		return _peek (arch);
	}

	template <typename Archive>
	typename Archive::int_type _skip_ws_and_peek (Archive & arch)
	{
		auto c = _peek (arch);
		while (_is_ws(c))
			c = _consume_and_peek (arch);

		return c;
	}

	template <typename Archive>
	typename Archive::int_type _consume_and_skip_ws_and_peek (Archive & arch)
	{
		typename Archive::int_type c;
		do {
			c = _consume_and_peek (arch);
		} while (_is_ws(c));

		return c;
	}

	template <typename Archive>
	bool _read_exact_str (Archive & arch, char const * s)
	{
		auto c = _peek (arch);
		for (unsigned i = 0; s[i]; ++i)
			if (s[i] != c) return false;
			else c = _consume_and_peek (arch);

		return true;
	}

	// Supports (and assumes) only decimal literals
	template <typename Archive>
	Int _read_int (Archive & arch)
	{
		Int ret = 0;
		bool negative = false;

		auto c = _peek (arch);
		if ('-' == c || '+' == c)
		{
			negative = ('-' == c);
			c = _consume_and_peek (arch);
		}
		while (_is_digit(c))
		{
			ret = 10 * ret + (c - '0');
			c = _consume_and_peek (arch);
		}

		if (negative)
			ret = -ret;

		return ret;
	}

	template <typename Archive>
	Real _read_real (Archive & arch, bool & really_real, Int & integer_part)
	{
		really_real = false;

		Int int_part = 0;
		Real ret = 0;
		bool negative = false;

		auto c = _peek (arch);
		if ('-' == c || '+' == c)
		{
			negative = ('-' == c);
			c = _consume_and_peek (arch);
		}
		while (_is_digit(c))
		{
			int_part = 10 * int_part + (c - '0');
			ret = 10 * ret + (c - '0');
			c = _consume_and_peek (arch);
		}

		if ('.' == c)
		{
			really_real = true;
			long double mul = 1;
			c = _consume_and_peek (arch);
			while (_is_digit(c))
			{
				mul /= 10;
				ret = Real(ret + mul * (c - '0'));
				c = _consume_and_peek (arch);
			}
		}

		if ('e' == c || 'E' == c)
		{
			really_real = true;
			c = _consume_and_peek (arch);
			Int p = _read_int (arch);
			ret *= std::pow (10.0, p);
		}

		if (negative)
		{
			int_part = -int_part;
			ret = -ret;
		}
		integer_part = int_part;

		return ret;
	}

	template <typename Archive>
	String _read_str (Archive & arch)
	{
		String ret;
		auto c = _peek (arch);
		if (c != '"')
			return ret;
		c = _consume_and_peek (arch);
		while (c != '"')
		{
			if ('\\' == c)
			{
				c = _consume_and_peek (arch);
				switch (c)
				{
					case 'a': ret += '\a'; break;
					case 'b': ret += '\b'; break;
					case 'f': ret += '\f'; break;
					case 'n': ret += '\n'; break;
					case 'r': ret += '\r'; break;
					case 't': ret += '\t'; break;
					case 'v': ret += '\v'; break;
					default: ret += char(c); break;
				}
			}
			else
				ret += char(c);

			c = _consume_and_peek (arch);
		}
		_consume_and_peek (arch);

		return ret;
	}
};

//======================================================================

template <typename Archive>
Value Parse (Archive & arch)
{
	auto c = _parser_details::_skip_ws_and_peek (arch);

	if ('{' == c)
		return Value::ReadObject (arch);
	else if ('[' == c)
		return Value::ReadArray (arch);
	else if ('"' == c)
		return Value::ReadString (arch);
	else if ('t' == c || 'f' == c)
		return Value::ReadBool (arch);
	else if ('n' == c)
		return Value::ReadNull (arch);
	else if (_parser_details::_is_digit(c) || '-' == c || '+' == c || '.' == c)
		return Value::ReadNumber (arch);
	else
		return Value();
}

//======================================================================

template <typename Archive>
void Value::WriteString (Archive & arch, String const & str)
{
	arch << '"';
	for (auto c : str)
		switch (c)
		{
			case '"': arch << "\\\""; break;
			case '\\': arch << "\\\\"; break;
			case '\t': arch << "\\t"; break;
			case '\r': arch << "\\r"; break;
			case '\n': arch << "\\n"; break;
			default: arch << c; break;
		}
	arch << '"';
}

//----------------------------------------------------------------------

template <typename Archive>
void Value::WriteReal (Archive & arch, Real r)
{
	const unsigned BufSize = 50;

	char rs [BufSize + 1];
	int l = snprintf (rs, BufSize, "%0.20g", r);

	if (l > 0)
	{
		int i;
		for (i = l - 1; i > 0 && rs[i] == '0'; --i);
		if (rs[i] == '.') i += 1;
		rs[i + 1] = '\0';

		arch << rs;
	}
	else
		arch << "error-in-writing-real-value";
}

//----------------------------------------------------------------------

template <typename Archive>
Value Value::ReadNull (Archive & arch)
{
	_parser_details::_read_exact_str (arch, "null");
	return Value();
}

//----------------------------------------------------------------------

template <typename Archive>
Value Value::ReadBool (Archive & arch)
{
	auto c = _parser_details::_peek (arch);
	if ('t' == c && _parser_details::_read_exact_str(arch, "true"))
		return Value(true);
	else if ('f' == c && _parser_details::_read_exact_str(arch, "false"))
		return Value(false);
	else return Value();
}

//----------------------------------------------------------------------

template <typename Archive>
Value Value::ReadInt (Archive & arch)
{
	return Value(_parser_details::_read_int(arch));
}

//----------------------------------------------------------------------

template <typename Archive>
Value Value::ReadReal (Archive & arch)
{
	return Value(_parser_details::_read_real(arch));
}

//----------------------------------------------------------------------

template <typename Archive>
Value Value::ReadNumber (Archive & arch)
{
	bool is_real = false;
	Int i;
	Real r = _parser_details::_read_real (arch, is_real, i);

	if (is_real)
		return Value(r);
	else
		return Value(i);
}

//----------------------------------------------------------------------

template <typename Archive>
Value Value::ReadString (Archive & arch)
{
	auto c = _parser_details::_skip_ws_and_peek (arch);
	if (c != '"')
		return Value();
	else
		return Value(_parser_details::_read_str(arch));
}

//----------------------------------------------------------------------

template <typename Archive>
Value Value::ReadArray (Archive & arch)
{
	auto c = _parser_details::_skip_ws_and_peek (arch);
	if (c != '[')
		return Value();

	Value ret (Type::Array);

	c = _parser_details::_consume_and_skip_ws_and_peek (arch);
	while (']' != c)
	{
		ret.asArray().emplace_back (Parse(arch));

		c = _parser_details::_skip_ws_and_peek (arch);
		if (',' != c && ']' != c)
			return ret;
		if (',' == c)
			c = _parser_details::_consume_and_skip_ws_and_peek (arch);
	}
	_parser_details::_consume_and_peek (arch);

	return ret;
}

//----------------------------------------------------------------------

template <typename Archive>
Value Value::ReadObject (Archive & arch)
{
	return Value(Object::Read(arch));
}

//======================================================================

template <typename Archive>
Object Object::Read (Archive & arch)
{
	Object ret;

	auto c = _parser_details::_skip_ws_and_peek (arch);
	if (c != '{')
		return ret;

	c = _parser_details::_consume_and_skip_ws_and_peek (arch);
	while ('}' != c)
	{
		String name = _parser_details::_read_str (arch);
		c = _parser_details::_skip_ws_and_peek (arch);
		if (':' != c)
			return ret;
		_parser_details::_consume_and_peek (arch);
		Value value = Parse (arch);

		ret.add (std::move(name), std::move(value));

		c = _parser_details::_skip_ws_and_peek (arch);
		if (',' != c && '}' != c)
			return ret;
		if (',' == c)
			c = _parser_details::_consume_and_skip_ws_and_peek (arch);
	}
	_parser_details::_consume_and_peek (arch);

	return ret;
}

//======================================================================

	}	// namespace JSON
}	// namespace Kage

//======================================================================

#endif	// __KAGE__JSON_H__

#endif

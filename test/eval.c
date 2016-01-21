#include "tests.h"

#include <assert.h>
#include <stdlib.h>
#include "eval.h"
#include "objects/class.h"
#include "objects/str.h"
#include "objects/exception.h"
#include "gc.h"


# define assertEvalEq(expectedAstString, source)        \
  assertEvalEqImpl((expectedAstString), (source),       \
                   __func__, YOLO__POSITION_STRING)

# define assertException(exceptionClass, expectedMessage, source)       \
  assertExceptionImpl((exceptionClass), (expectedMessage), (source),    \
                      __func__, YOLO__POSITION_STRING)


static void assertEvalEqImpl(const char *expected,
                             const char *source,
                             const char *testName,
                             const char *position) {

  wsky_ReturnValue r = wsky_evalString(source);
  yolo_assert_ptr_eq_impl(NULL, r.exception, testName, position);
  if (r.exception) {
    printf("%s\n", r.exception->message);
    return;
  }
  wsky_ReturnValue stringRv = wsky_toString(r.v);
  yolo_assert_null(stringRv.exception);
  assert(wsky_isString(stringRv.v));
  wsky_String *string = (wsky_String *) stringRv.v.v.objectValue;
  yolo_assert_str_eq_impl(expected, string->string, testName, position);
}

static void assertExceptionImpl(const char *exceptionClass,
                                const char *expectedMessage,
                                const char *source,
                                const char *testName,
                                const char *position) {

  wsky_ReturnValue r = wsky_evalString(source);
  yolo_assert_ptr_neq_impl(NULL, r.exception, testName, position);
  if (!r.exception) {
    return;
  }
  yolo_assert_str_eq_impl(exceptionClass,
                          r.exception->class->name,
                          testName, position);
  yolo_assert_str_eq_impl(expectedMessage, r.exception->message,
                          testName, position);
}



static void syntaxError(void) {
  assertException("SyntaxError", "Unexpected end of file", "8 +");
  assertException("SyntaxError", "Expected end of string", "'");
}


static void literals(void) {
  assertEvalEq("123","123");

  assertEvalEq("123.0","123.0");
  assertEvalEq("123.456","123.456");
  assertEvalEq("1e+23","100000000000000000000000.0");
  assertEvalEq("1e+23","100000000000000000000000.0f");
  assertEvalEq("1e+23","100000000000000000000000f");

  assertEvalEq("true","true");
  assertEvalEq("false","false");
  assertEvalEq("null","null");
}

static void strings(void) {
  assertEvalEq("lol","'lol'");
  assertEvalEq("abcdef","'abc' + 'def'");

  assertEvalEq("abctrue","'abc' + true");
  assertEvalEq("abcfalse","'abc' + false");
  assertEvalEq("trueabc","true + 'abc'");
  assertEvalEq("falseabc","false + 'abc'");

  assertEvalEq("abc-12","'abc' + -12");
  assertEvalEq("abc-6.7","'abc' + -6.7");
  assertEvalEq("-12abc","-12 + 'abc'");
  assertEvalEq("-6.7abc","-6.7 + 'abc'");

  assertEvalEq("ababab","'ab' * 3");
  assertEvalEq("ababab","3 * 'ab'");
  assertEvalEq("","0 * 'abc'");
  assertEvalEq("","3 * ''");

  assertException("ValueError", "The factor cannot be negative",
                  "-3 * 'abc'");
}

static void unaryOps(void) {
  assertException("TypeError", "Unsupported class for unary -: String",
                  "-'abc'");

  assertEvalEq("-1", "-1");
  assertEvalEq("-1", "-+1");
  assertEvalEq("-1", "-+++--1");
  assertEvalEq("1", "-++-+--1");
  assertEvalEq("-56.0", "-56.0");
  assertEvalEq("-56.0", "-+56.0");
  assertEvalEq("56.0", "-+-56.0");
}

static void binaryOps(void) {
  assertException("TypeError",
                  "Unsupported classes for -: String and String",
                  "'def' - 'abc'");

  assertEvalEq("2", "1 + 1");
  assertEvalEq("20", "4 * 5");
  assertEvalEq("-1", "4 - 5");
  assertEvalEq("113", "567 / 5");

  assertEvalEq("2.0", "1 + 1.0");
  assertEvalEq("20.0", "4 * 5.0");
  assertEvalEq("-1.0", "4 - 5.0");
  assertEvalEq("113.4", "567 / 5.0");

  assertEvalEq("2.0", "1.0 + 1");
  assertEvalEq("20.0", "4.0 * 5");
  assertEvalEq("-1.0", "4.0 - 5");
  assertEvalEq("113.4", "567.0 / 5");

  assertEvalEq("2.0", "1.0 + 1.0");
  assertEvalEq("20.0", "4.0 * 5.0");
  assertEvalEq("-1.0", "4.0 - 5.0");
  assertEvalEq("113.4", "567.0 / 5.0");
}

static void binaryCmpOps(void) {
  assertEvalEq("false", "567 == 56");
  assertEvalEq("true", "567 == 567");
  assertEvalEq("false", "567 != 567");
  assertEvalEq("true", "567 != 566");
  assertEvalEq("true", "567 != 568");

  assertEvalEq("false", "566 > 566");
  assertEvalEq("true", "567 > 566");
  assertEvalEq("false", "566 > 567");

  assertEvalEq("false", "566.0 > 566");
  assertEvalEq("true", "567.0 > 566");
  assertEvalEq("false", "566.0 > 567");

  assertEvalEq("false", "566 > 566.0");
  assertEvalEq("true", "567 > 566.0");
  assertEvalEq("false", "566 > 567.0");

  assertEvalEq("false", "566.0 > 566.0");
  assertEvalEq("true", "567.0 > 566.0");
  assertEvalEq("false", "566.0 > 567.0");

  assertEvalEq("true", "566 >= 566");
  assertEvalEq("true", "567 >= 566");
  assertEvalEq("false", "566 >= 567");

  assertEvalEq("false", "566 < 566");
  assertEvalEq("false", "567 < 566");
  assertEvalEq("true", "566 < 567");

  assertEvalEq("true", "566 <= 566");
  assertEvalEq("false", "567 <= 566");
  assertEvalEq("true", "566 <= 567");
}

static void binaryBoolOps(void) {
  assertEvalEq("true", "not false");
  assertEvalEq("false", "not not false");
  assertEvalEq("true", "not not not false");

  assertEvalEq("true", "true and true");
  assertEvalEq("false", "false and true");
  assertEvalEq("false", "true and false");
  assertEvalEq("false", "false and false");

  assertEvalEq("true", "true or true");
  assertEvalEq("true", "false or true");
  assertEvalEq("true", "true or false");
  assertEvalEq("false", "false or false");

  assertEvalEq("true", "not true or true");
  assertEvalEq("true", "(not true) or true");
  assertEvalEq("true", "true or not true");
  assertEvalEq("true", "true or (not true)");
  assertEvalEq("false", "not (true or true)");
}

static void sequence(void) {
  assertEvalEq("12", "12");
  assertEvalEq("12", "12;");
  assertEvalEq("12", "678;12;");

  assertEvalEq("12", "(12)");
  assertEvalEq("12", "(12;)");
  assertEvalEq("12", "(678;12;)");

  assertException("SyntaxError", "Expected ')'", "(");
  assertException("SyntaxError", "Expected ')'", "(123");
  assertException("SyntaxError", "Expected ')'", "(123;");
  assertException("SyntaxError", "Expected ';' or ')'", "(123 456)");
}

static void var(void) {
  assertException("SyntaxError", "Expected variable name", "var");
  assertException("SyntaxError", "Expected variable name", "var 123");
  assertException("SyntaxError", "Unexpected ';'", "var a = ;");

  assertEvalEq("null", "var a");
  assertEvalEq("45", "var a = 45");
}

static void variable(void) {
  assertEvalEq("69", "var a = 67; a + 2");
  assertEvalEq("2", "var a = 12; a = 2");
  assertEvalEq("2", "var a = 12; a = 2; a");
  assertEvalEq("-4", "var a = 12; a = 2; a - 6");
}

static void scope(void) {
  assertEvalEq("2",
               "var a = 1;"
               "("
               "    var a = 2;"
               "    a"
               ")");

  assertEvalEq("2",
               "("
               "    var a = 1;"
               "    ("
               "        var a = 2;"
               "        a"
               "    )"
               ")");
}

static void function(void) {
  assertEvalEq("<Function>", "{}");
  assertEvalEq("<Function>", "{ a, b, c: 'yolo'}");
  assertEvalEq("<Function>", "{{{}}}");

  assertException("SyntaxError", "Expected '}'", "{");
  assertException("SyntaxError", "Expected ';' or '}'", "{a b}");
}

static void call(void) {
  assertException("SyntaxError", "Expected ')'", "0(");
  assertException("SyntaxError", "Expected ',' or ')'", "0(a b)");

  assertEvalEq("null", "{}()");
  assertEvalEq("1", "{1}()");
  assertEvalEq("1", "{{1}}()()");
  assertEvalEq("lol", "{'lol'}()");
  assertEvalEq("34", "{31}() + 3");

  assertEvalEq("<Function>",
               "var a = {}");

  assertEvalEq("34",
               "var a = {31};"
               "a() + 3");

  assertEvalEq("34",
               "var f = {a: a};"
               "f(31) + 3");

  assertEvalEq("3",
               "var a = 3;"
               " {a}()");

  assertEvalEq("5",
               "{a:"
               "    {b: a + b}"
               "}(2)(3)");

  assertEvalEq("5",
               "var f = {a:"
               "    {b: a + b}"
               "};"
               "f(2)(3)");

  assertEvalEq("7",
               "{a, b, c: a + b * c}(1, 2, 3)");
}

static void functionScope(void) {
  assertEvalEq("2", "var a = 1; {var a = 2; a}()");
  assertEvalEq("1", "var a = 1; {var a = 2}(); a");
}

static void method(void) {
  assertEvalEq("1",
               "var m = 'hello'.indexOf;"
               "m('e')");

  assertEvalEq("1", "'hello'.indexOf('e')");

  assertException("AttributeError", "Integer object has no attribute vodka",
                  "0.vodka");
}

static void toString(void) {
  assertEvalEq("whiskey", "'whiskey'.toString");
  assertEvalEq("<Function>", "{}.toString");
  assertEvalEq("null", "null.toString");
  assertEvalEq("null", "().toString");
  assertEvalEq("true", "true.toString");
  assertEvalEq("false", "false.toString");
  assertEvalEq("0", "0.toString");
  assertEvalEq("123", "0123.toString");
  assertEvalEq("0.0", "0.0.toString");
  assertEvalEq("123.4", "123.4.toString");
  assertEvalEq("<Class String>", "''.class.toString");

  assertEvalEq("<Class Integer>", "0.class");

  assertEvalEq("<Function>", "'' + {}");
  assertEvalEq("<Function>", "{} + ''");
  assertEvalEq("<Class Function>", "{}.class + ''");
  assertEvalEq("null", "null + ''");
  assertEvalEq("null", "() + ''");
  assertEvalEq("true", "true + ''");
  assertEvalEq("false", "false + ''");
  assertEvalEq("0", "0 + ''");
  assertEvalEq("123", "0123 + ''");
  assertEvalEq("0.0", "0.0 + ''");
  assertEvalEq("123.4", "123.4 + ''");
}

static void getClass(void) {
  assertEvalEq("<Class Function>", "{}.class");

  assertEvalEq("<Class String>", "''.class");
  assertEvalEq("<Class Integer>", "0.class");
  assertEvalEq("<Class Float>", "0.0.class");
  assertEvalEq("<Class NullClass>", "null.class");
  assertEvalEq("<Class Class>", "null.class.class.class.class.class");
}

static void objectEquals(void) {
  assertException("TypeError",
                  "Unsupported classes for ==: Function and Function",
                  "{} == {}");
}

static void string(void) {
  assertEvalEq("0", "''.length");
  assertEvalEq("3", "'abc'.length");

  assertEvalEq("3", "'abc'.length");
}

static void class(void) {
  assertEvalEq("<Class Duck>", "class Duck ()");
  assertEvalEq("<Class Duck>", "(class Duck ()).toString");
  assertEvalEq("<Duck>", "(class Duck ())()");
  assertEvalEq("<Duck>", "(class Duck (init {}))()");
}

void evalTestSuite(void) {
  syntaxError();

  literals();
  strings();

  unaryOps();
  binaryOps();
  binaryCmpOps();
  binaryBoolOps();
  sequence();
  var();
  variable();
  scope();
  function();
  call();
  functionScope();
  method();
  toString();
  getClass();
  objectEquals();
  string();
  class();

  wsky_GC_unmarkAll();
  wsky_GC_visitBuiltins();
  wsky_GC_collect();
}

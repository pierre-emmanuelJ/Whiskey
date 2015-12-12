#include "tests.h"

#include <string.h>
#include "../class.h"
#include "../gc.h"
#include "../string_reader.h"


static void empty(void) {
  wsky_StringReader *reader;

  reader = wsky_StringReader_newStr("");
  yolo_assert(!wsky_StringReader_hasMore(reader));
  yolo_assert_int_eq(1, reader->position.line);
  yolo_assert_int_eq(0, reader->position.column);
  wsky_StringReader_delete(reader);
}

static void newLine(void) {
  wsky_StringReader *reader;

  reader = wsky_StringReader_newStr("a\n");
  yolo_assert_int_eq(1, reader->position.line);
  yolo_assert_int_eq(0, reader->position.column);
  yolo_assert(wsky_StringReader_hasMore(reader));

  yolo_assert_char_eq('a', wsky_StringReader_next(reader));
  yolo_assert(wsky_StringReader_hasMore(reader));
  yolo_assert_int_eq(1, reader->position.line);
  yolo_assert_int_eq(1, reader->position.column);

  yolo_assert_char_eq('\n', wsky_StringReader_next(reader));
  yolo_assert(!wsky_StringReader_hasMore(reader));
  yolo_assert_int_eq(2, reader->position.line);
  yolo_assert_int_eq(0, reader->position.column);

  wsky_StringReader_delete(reader);
}

static void skipping(void) {
  wsky_StringReader *reader;
  reader = wsky_StringReader_newStr("a\n\r\t \t   b");

  yolo_assert_char_eq('a', wsky_StringReader_next(reader));
  yolo_assert(wsky_StringReader_hasMore(reader));

  wsky_StringReader_skipWhitespaces(reader);

  yolo_assert_char_eq('b', wsky_StringReader_next(reader));
  yolo_assert(!wsky_StringReader_hasMore(reader));

  wsky_StringReader_delete(reader);
}

static void skipping2(void) {
  wsky_StringReader *reader;
  reader = wsky_StringReader_newStr("a\n\r  \t  ");

  yolo_assert_char_eq('a', wsky_StringReader_next(reader));
  yolo_assert(wsky_StringReader_hasMore(reader));

  wsky_StringReader_skipWhitespaces(reader);

  yolo_assert(!wsky_StringReader_hasMore(reader));

  wsky_StringReader_delete(reader);
}

static void readString(void) {
  wsky_StringReader *reader;
  reader = wsky_StringReader_newStr("abcdef");

  yolo_assert(wsky_StringReader_readString(reader, "abcde"));
  yolo_assert_char_eq('f', wsky_StringReader_next(reader));

  wsky_StringReader_delete(reader);
}

static void token(void) {
  wsky_StringReader *reader;
  reader = wsky_StringReader_newStr("a bc d");

  wsky_StringReader_next(reader);
  yolo_assert_char_eq(' ', wsky_StringReader_next(reader));
  wsky_Position begin = reader->position;
  yolo_assert_char_eq('b', wsky_StringReader_next(reader));
  yolo_assert_char_eq('c', wsky_StringReader_next(reader));
  wsky_Token t;
  t = wsky_StringReader_createToken(reader, begin, wsky_TokenType_COMMENT);
  yolo_assert_str_eq("bc", t.string);
  wsky_Token_free(&t);
  yolo_assert_char_eq(' ', wsky_StringReader_next(reader));

  wsky_StringReader_delete(reader);
}

void stringReaderTestSuite(void) {
  empty();
  newLine();
  skipping();
  skipping2();
  readString();
  token();
}

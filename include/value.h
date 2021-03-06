#ifndef VALUE_H_
# define VALUE_H_

#include "type.h"
#include <stdarg.h>

typedef struct wsky_Class_s wsky_Class;
typedef struct wsky_String_s wsky_String;
typedef struct wsky_Object_s wsky_Object;

/**
 * @defgroup Value Value
 * @{
 */

/**
 * The type of a wsky_Value.
 */
typedef enum {
  wsky_Type_BOOL,
  wsky_Type_INT,
  wsky_Type_FLOAT,
  wsky_Type_OBJECT
} wsky_Type;

/**
 * A Whiskey value.
 *
 * Integers, booleans and floats are not objects, and are not
 * garbage-collected.
 * This structure can hold any Whiskey value, whatever its type.
 */
typedef struct wsky_Value_s {

  /** An union of the differents types */
  union {
    /** If type == wsky_Type_BOOL */
    bool boolValue;

    /** If type == wsky_Type_INT */
    wsky_int intValue;

    /** If type == wsky_Type_FLOAT */
    wsky_float floatValue;

    /** If type == wsky_Type_OBJECT */
    wsky_Object *objectValue;
  } v;

  /** The type of the value */
  wsky_Type type;

} wsky_Value;


/** A predefined value for `true` */
extern const wsky_Value wsky_Value_TRUE;

/** A predefined return value for `false` */
extern const wsky_Value wsky_Value_FALSE;

/** A predefined return value for `null` */
extern const wsky_Value wsky_Value_NULL;

/** Creates a new value from a boolean */
static inline wsky_Value wsky_Value_fromBool(bool n) {
  return n ? wsky_Value_TRUE : wsky_Value_FALSE;
}

/** Creates a new value from a wsky_Object */
static inline wsky_Value wsky_Value_fromObject(wsky_Object *object) {
  wsky_Value v = {
    .type = wsky_Type_OBJECT,
    .v = {
      .objectValue = object
    }
  };
  return v;
}

/** Creates a new value from an integer */
static inline wsky_Value wsky_Value_fromInt(wsky_int n) {
  wsky_Value v = {
    .type = wsky_Type_INT,
    .v = {
      .intValue = n
    }
  };
  return v;
}

/** Creates a new value from a float */
static inline wsky_Value wsky_Value_fromFloat(wsky_float n) {
  wsky_Value v = {
    .type = wsky_Type_FLOAT,
    .v = {
      .floatValue = n
    }
  };
  return v;
}

/** Returns a wsky_Value or NULL */
wsky_Value *wsky_Value_new(wsky_Value v);

/**
 * Return `true` if the type of the given value is OBJECT and its
 * member objectValue is NULL
 */
static inline bool wsky_isNull(const wsky_Value value) {
  return value.type == wsky_Type_OBJECT && !value.v.objectValue;
}

/**
 * Return the class of the given value.
 */
wsky_Class *wsky_getClass(const wsky_Value value);

/**
 * Returns the class name of the given value.
 */
const char *wsky_getClassName(const wsky_Value value);



/**
 * Like wsky_buildValue(), but with a va_list.
 */
wsky_Value wsky_vaBuildValue(const char *format, va_list parameters);

/**
 * Like wsky_buildValues(), but with a single value.
 */
wsky_Value wsky_buildValue(const char *format, ...);

/**
 * Builds Whiskey values from C values and a format string,
 * a bit like printf().
 *
 * The format string can contain the characters:
 *   - i: Integer
 *   - f: Float
 *   - s: String
 */
int wsky_buildValues(const wsky_Value *values, const char *format, ...);



/**
 * Like wsky_parseValues(), but with a va_list.
 */
int wsky_vaParseValues(const wsky_Value *values, const char *format,
                       va_list parameters);

/**
 * Converts C values to a Whiskey values with a format string,
 * a bit like scanf().
 *
 * The format string can contain the characters:
 *   - i: Integer
 *   - f: Float
 *   - s: Copy the null-terminated string to the character array whose
 *   address you pass.
 *   - S: Store a pointer to a malloc'd string in the character pointer
 *   variable whose address you pass. You must free the string.
 */
int wsky_parseValues(const wsky_Value *values, const char *format, ...);

/**
 * @}
 */

#endif /* !VALUE_H_ */

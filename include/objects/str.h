#ifndef STR_H_
# define STR_H_

# include "object.h"
# include "class_def.h"

/**
 * @addtogroup objects
 * @{
 *
 * @defgroup String String
 * @{
 */


extern const wsky_ClassDef wsky_String_CLASS_DEF;

extern wsky_Class *wsky_String_CLASS;


/** A Whiskey string */
struct wsky_String_s {
  wsky_OBJECT_HEAD

  /** The underlying malloc'd C string */
  char *string;
};



wsky_String *wsky_String_new(const char *cString);

static inline bool wsky_isString(const wsky_Value value) {
  if (value.type != wsky_Type_OBJECT)
    return false;
  if (wsky_isNull(value))
    return false;
  return value.v.objectValue->class == wsky_String_CLASS;
}

wsky_ReturnValue wsky_String_equals(wsky_String *self,
                                      wsky_Value other);
wsky_ReturnValue wsky_String_startsWith(wsky_String *self,
                                          wsky_Value otherV);
wsky_ReturnValue wsky_String_indexOf(wsky_String *self,
                                       wsky_Value otherV);
wsky_ReturnValue wsky_String_contains(wsky_String *self,
                                        wsky_Value otherV);

void wsky_String_print(const wsky_String *self);



char *wsky_String_escapeCString(const char *source);

/**
 * @}
 * @}
 */

#endif /* !STR_H_ */

#include "../return_value_private.h"


typedef wsky_Object Object;
typedef wsky_Value Value;
typedef wsky_String String;


static ReturnValue toString(Value *self);


#define M(name, flags, paramCount)                                      \
  {#name, paramCount, wsky_MethodFlags_VALUE | flags, (wsky_Method0)&name}

static wsky_MethodDef methods[] = {
  M(toString, wsky_MethodFlags_GET | wsky_MethodFlags_PUBLIC, 0),
  {0, 0, 0, 0},
};

#undef M


const wsky_ClassDef wsky_Null_CLASS_DEF = {
  .super = &wsky_Object_CLASS_DEF,
  .name = "NullClass",
  .final = true,
  .constructor = NULL,
  .destructor = NULL,
  .objectSize = 0,
  .methodDefs = methods,
  .gcAcceptFunction = NULL,
};

wsky_Class *wsky_Null_CLASS;



static ReturnValue toString(Value *self) {
  (void) self;
  RETURN_C_STRING("null");
}

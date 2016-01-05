#include "objects/class.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "gc.h"
#include "return_value.h"
#include "objects/str.h"


typedef wsky_Class Class;
typedef wsky_ClassDef ClassDef;
typedef wsky_Object Object;
typedef wsky_Value Value;
typedef wsky_ReturnValue ReturnValue;


static ReturnValue construct(Object *object,
                             unsigned paramCount,
                             Value *params);
static ReturnValue destroy(Object *object);


static ReturnValue toString(Class *self);


#define GET(name)                                       \
  {#name, 0, wsky_MethodFlags_GET, (void *) &name}

static wsky_MethodDef methods[] = {
  GET(toString),

  {0, 0, 0, 0},
};

#undef GET


const ClassDef wsky_Class_CLASS_DEF = {
  .super = &wsky_Object_CLASS_DEF,
  .name = "Class",
  .constructor = &construct,
  .destructor = &destroy,
  .objectSize = sizeof(Class),
  .methodDefs = methods,
  .gcAcceptFunction = NULL,
};

wsky_Class *wsky_Class_CLASS;



static void initMethods(Class *class, const ClassDef *def) {
  class->methods = wsky_Dict_new();
  wsky_MethodDef *methodDef = def->methodDefs;
  while (methodDef->name) {
    wsky_MethodObject* method = wsky_MethodObject_newFromC(methodDef);
    wsky_Dict_set(class->methods, method->name, method);
    methodDef++;
  }
}


Class *wsky_Class_new(const ClassDef *def, Class *super) {
  Class *class = wsky_MALLOC(sizeof(Class));
  if (!class)
    return NULL;
  class->class = wsky_Class_CLASS;
  wsky_GC_register((Object *) class);
  class->name = wsky_STRDUP(def->name);
  class->objectSize = def->objectSize;
  class->super = super;
  class->destructor = def->destructor;

  if (def == &wsky_Class_CLASS_DEF ||
      def == &wsky_Object_CLASS_DEF ||
      def == &wsky_MethodObject_CLASS_DEF) {
    class->constructor = NULL;
  } else {
    initMethods(class, def);

    wsky_MethodDef ctorDef = {
      "<Constructor>",
      -1,
      wsky_MethodFlags_PUBLIC,
      (void *) def->constructor,
    };
    class->constructor = wsky_MethodObject_newFromC(&ctorDef);
  }
  return class;
}

static wsky_ReturnValue construct(Object *object,
                                  unsigned paramCount,
                                  Value *params) {
  (void) paramCount;
  (void) params;
  (void) object;
  wsky_RETURN_NULL;
}

static wsky_ReturnValue destroy(Object *object) {
  Class *self = (Class *) object;
  wsky_FREE(self->name);
  if (self->constructor)
    wsky_Dict_delete(self->methods);
  wsky_RETURN_NULL;
}


static ReturnValue toString(Class *self) {
  (void) self;
  wsky_RETURN_CSTRING("<Class>");

}


static void acceptGcOnField(const char* name, void *value_) {
  (void) name;
  wsky_GC_VISIT_VALUE(*(Value *) value_);
}

void wsky_Class_acceptGC(wsky_Object *object) {
  wsky_GC_VISIT(object->class);
  wsky_Dict_apply(object->fields, &acceptGcOnField);
}
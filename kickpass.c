/*
 * Copyright (c) 2015 Paul Fariello <paul@fariello.eu>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <Python.h>
#include "structmember.h"
#include <kickpass/kickpass.h>
#include <kickpass/safe.h>

typedef struct {
	PyObject_HEAD;
	struct kp_ctx ctx;
} Context;

typedef struct {
	PyObject_HEAD;
	Context *context;
	struct kp_safe safe;
} Safe;

static PyMemberDef Safe_members[] = {
	{"context", T_OBJECT_EX, offsetof(Safe, context), 0, "context"},
	{NULL}
};

static PyObject *
Context_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	Context *self;

	self = (Context *)type->tp_alloc(type, 0);
	if (self == NULL) {
		return NULL;
	}

	if (kp_init(&self->ctx) != KP_SUCCESS) {
		Py_DECREF(self);
		return NULL;
	}

	return (PyObject *)self;
}

static void
Context_dealloc(Context *self)
{
	kp_fini(&self->ctx);
	Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyTypeObject ContextType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"kickpass.Context",          /* tp_name */
	sizeof(Context),             /* tp_basicsize */
	0,                           /* tp_itemsize */
	(destructor)Context_dealloc, /* tp_dealloc */
	0,                           /* tp_print */
	0,                           /* tp_getattr */
	0,                           /* tp_setattr */
	0,                           /* tp_reserved */
	0,                           /* tp_repr */
	0,                           /* tp_as_number */
	0,                           /* tp_as_sequence */
	0,                           /* tp_as_mapping */
	0,                           /* tp_hash */
	0,                           /* tp_call */
	0,                           /* tp_str */
	0,                           /* tp_getattro */
	0,                           /* tp_setattro */
	0,                           /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,          /* tp_flags */
	"Kickpass context",          /* tp_doc */
	0,                           /* tp_traverse */
	0,                           /* tp_clear */
	0,                           /* tp_richcompare */
	0,                           /* tp_weaklistoffset */
	0,                           /* tp_iter */
	0,                           /* tp_iternext */
	0,                           /* tp_methods */
	0,                           /* tp_members */
	0,                           /* tp_getset */
	0,                           /* tp_base */
	0,                           /* tp_dict */
	0,                           /* tp_descr_get */
	0,                           /* tp_descr_set */
	0,                           /* tp_dictoffset */
	0,                           /* tp_init */
	0,                           /* tp_alloc */
	Context_new,                 /* tp_new */
};

static PyObject *
Safe_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	Safe *self;

	self = (Safe *)type->tp_alloc(type, 0);
	if (self == NULL) {
		return NULL;
	}

	self->context = NULL;

	return (PyObject *)self;
}

static int
Safe_init(Safe *self, PyObject *args, PyObject *kwds)
{
	PyObject *opath;
	Context *context;
	const char *path;

	static char *kwlist[] = {"context", "path", NULL};

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O!O&", kwlist,
	                                 &ContextType, &context,
	                                 PyUnicode_FSConverter, &opath)) {
		return -1;
	}

	self->context = context;
	path = PyBytes_AsString(opath);
	Py_DECREF(opath);

	if (kp_safe_init(&self->context->ctx, &self->safe, path)
	    != KP_SUCCESS) {
		return -1;
	}

	return 0;
}

static PyObject *
Safe_open(Safe *self)
{
	if (kp_safe_open(&self->context->ctx, &self->safe, 0) != KP_SUCCESS) {
		return NULL;
	}

	return Py_None;
}

static PyMethodDef Safe_methods[] = {
	{"open", (PyCFunction)Safe_open, METH_NOARGS, "Open safe"},
	{NULL}
};

static void
Safe_dealloc(Safe *self)
{
	kp_safe_close(&self->context->ctx, &self->safe);
	Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyTypeObject SafeType = {
	PyVarObject_HEAD_INIT(NULL, 0)
	"kickpass.Safe",             /* tp_name */
	sizeof(Safe),                /* tp_basicsize */
	0,                           /* tp_itemsize */
	(destructor)Safe_dealloc,    /* tp_dealloc */
	0,                           /* tp_print */
	0,                           /* tp_getattr */
	0,                           /* tp_setattr */
	0,                           /* tp_reserved */
	0,                           /* tp_repr */
	0,                           /* tp_as_number */
	0,                           /* tp_as_sequence */
	0,                           /* tp_as_mapping */
	0,                           /* tp_hash  */
	0,                           /* tp_call */
	0,                           /* tp_str */
	0,                           /* tp_getattro */
	0,                           /* tp_setattro */
	0,                           /* tp_as_buffer */
	Py_TPFLAGS_DEFAULT,          /* tp_flags */
	"Kickpass safe",             /* tp_doc */
	0,                           /* tp_traverse */
	0,                           /* tp_clear */
	0,                           /* tp_richcompare */
	0,                           /* tp_weaklistoffset */
	0,                           /* tp_iter */
	0,                           /* tp_iternext */
	Safe_methods,                /* tp_methods */
	Safe_members,                /* tp_members */
	0,                           /* tp_getset */
	0,                           /* tp_base */
	0,                           /* tp_dict */
	0,                           /* tp_descr_get */
	0,                           /* tp_descr_set */
	0,                           /* tp_dictoffset */
	(initproc)Safe_init,         /* tp_init */
	0,                           /* tp_alloc */
	Safe_new,                 /* tp_new */
};

static PyObject *
kickpass_version(PyObject *self, PyObject *args)
{
	return PyUnicode_FromString(kp_version_string());
}

static PyMethodDef kickpass_methods[] = {
	{"version", kickpass_version, METH_NOARGS, "kickpass version"},
	{NULL, NULL, 0, NULL}
};

static struct PyModuleDef kickpass_module = {
	PyModuleDef_HEAD_INIT,
	"kickpass",
	NULL,
	-1,
	kickpass_methods
};

PyMODINIT_FUNC
PyInit_kickpass(void)
{
	PyObject* m;

	m = PyModule_Create(&kickpass_module);

	if (PyType_Ready(&ContextType) < 0) {
		return NULL;
	}

	if (PyType_Ready(&SafeType) < 0) {
		return NULL;
	}

	PyModule_AddObject(m, "Context", (PyObject *)&ContextType);
	PyModule_AddObject(m, "Safe", (PyObject *)&SafeType);

	return m;
}

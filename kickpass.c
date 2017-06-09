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

#include <string.h>
#include <stdio.h>

#include <Python.h>
#include "structmember.h"

#include <kickpass/kickpass.h>
#include <kickpass/safe.h>

#define KP_PYTHON_ERR -1

PyObject *exception;

static kp_error_t prompt_wrapper(struct kp_ctx *, bool, char *, const char *, va_list);
static void raise_kp_exception(kp_error_t);

typedef struct {
	PyObject_HEAD;
	struct kp_ctx ctx;
	PyObject *prompt;
} Context;

static PyObject *
Context_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
	kp_error_t ret;
	Context *self;

	self = (Context *)type->tp_alloc(type, 0);
	if (self == NULL) {
		return NULL;
	}

	if ((ret = kp_init(&self->ctx)) != KP_SUCCESS) {
		raise_kp_exception(ret);
		Py_DECREF(self);
		return NULL;
	}

	return (PyObject *)self;
}

static int
Context__init__(Context *self, PyObject *args, PyObject *kwds)
{
	PyObject *prompt;

	static char *kwlist[] = {"prompt", NULL};

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &prompt)) {
		return -1;
	}

	self->ctx.password_prompt = prompt_wrapper;
	self->prompt = prompt;

	return 0;
}

static void
Context_dealloc(Context *self)
{
	kp_fini(&self->ctx);
	Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Context_init(Context *self, PyObject *args, PyObject *kwds)
{
	kp_error_t ret;
	PyObject *opath = NULL;
	const char *path = "";

	static char *kwlist[] = {"path", NULL};

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "|O&", kwlist,
	                                 PyUnicode_FSConverter, &opath)) {
		return NULL;
	}

	if (opath != NULL) {
		path = PyBytes_AsString(opath);
		Py_DECREF(opath);
	}

	if ((ret = kp_init_workspace(&self->ctx, path))
	    != KP_SUCCESS) {
		raise_kp_exception(ret);
		return NULL;
	}

	return Py_None;
}

static PyMethodDef Context_methods[] = {
	{"init", (PyCFunction)Context_init, METH_VARARGS | METH_KEYWORDS,
	 "Initialize a new workspace"},
	{NULL}
};

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
	Context_methods,             /* tp_methods */
	0,                           /* tp_members */
	0,                           /* tp_getset */
	0,                           /* tp_base */
	0,                           /* tp_dict */
	0,                           /* tp_descr_get */
	0,                           /* tp_descr_set */
	0,                           /* tp_dictoffset */
	(initproc)Context__init__,   /* tp_init */
	0,                           /* tp_alloc */
	Context_new,                 /* tp_new */
};

typedef struct {
	PyObject_HEAD;
	Context *context;
	struct kp_safe safe;
} Safe;

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
Safe__init__(Safe *self, PyObject *args, PyObject *kwds)
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
Safe_open(Safe *self, PyObject *args, PyObject *kwds)
{
	kp_error_t ret;
	bool create = false, force = false;
	int flags = 0;

	static char *kwlist[] = {"create", "force", NULL};

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "|pp", kwlist,
	                                 &create, &force)) {
		return NULL;
	}

	if (create) {
		flags |= KP_CREATE;
	}
	if (force) {
		flags |= KP_FORCE;
	}

	if ((ret = kp_safe_open(&self->context->ctx, &self->safe, flags))
	    != KP_SUCCESS) {
		raise_kp_exception(ret);
		return NULL;
	}

	return Py_None;
}

static PyObject *
Safe_close(Safe *self)
{
	kp_error_t ret;

	if ((ret = kp_safe_close(&self->context->ctx, &self->safe))
	    != KP_SUCCESS) {
		raise_kp_exception(ret);
		return NULL;
	}

	return Py_None;
}

static PyObject *
Safe_save(Safe *self)
{
	kp_error_t ret;

	if ((ret = kp_safe_save(&self->context->ctx, &self->safe))
	    != KP_SUCCESS) {
		raise_kp_exception(ret);
		return NULL;
	}

	return Py_None;
}

static PyObject *
Safe_delete(Safe *self)
{
	kp_error_t ret;

	if ((ret = kp_safe_delete(&self->context->ctx, &self->safe))
	    != KP_SUCCESS) {
		raise_kp_exception(ret);
		return NULL;
	}

	return Py_None;
}

static void
Safe_dealloc(Safe *self)
{
	kp_safe_close(&self->context->ctx, &self->safe);
	Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *
Safe_password_getter(PyObject *_self, void *closure)
{
	Safe *self = (Safe *)_self;

	if (!self->safe.open) {
		return Py_None;
	}

	return PyBytes_FromString(self->safe.password);
}

static int
Safe_password_setter(PyObject *_self, PyObject *opassword, void *closure)
{
	Safe *self = (Safe *)_self;
	char *password;

	/* TODO ensure safe is open */
	password = PyBytes_AsString(opassword);
	if (password == NULL) {
		return -1;
	}

	if (strlcpy(self->safe.password, password, KP_PASSWORD_MAX_LEN) >= KP_PASSWORD_MAX_LEN) {
		errno = ENOMEM;
		raise_kp_exception(KP_ERRNO);
		return -1;
	}

	return 0;
}

static PyObject *
Safe_metadata_getter(PyObject *_self, void *closure)
{
	Safe *self = (Safe *)_self;

	if (!self->safe.open) {
		return Py_None;
	}

	return PyBytes_FromString(self->safe.metadata);
}

static int
Safe_metadata_setter(PyObject *_self, PyObject *ometadata, void *closure)
{
	Safe *self = (Safe *)_self;
	char *metadata;

	/* TODO ensure safe is open */
	metadata = PyBytes_AsString(ometadata);
	if (metadata == NULL) {
		return -1;
	}

	if (strlcpy(self->safe.metadata, metadata, KP_PASSWORD_MAX_LEN) >= KP_PASSWORD_MAX_LEN) {
		errno = ENOMEM;
		raise_kp_exception(KP_ERRNO);
		return -1;
	}

	return 0;
}

static PyObject *
Safe_path_getter(PyObject *_self, void *closure)
{
	kp_error_t ret;
	char path[PATH_MAX] = "";
	Safe *self = (Safe *)_self;

	if ((ret = kp_safe_get_path(&self->context->ctx, &self->safe, path,
	                            PATH_MAX)) != KP_SUCCESS) {
		raise_kp_exception(ret);
		return NULL;
	}

	return PyUnicode_DecodeFSDefault(path);
}

static PyObject *
Safe_name_getter(PyObject *_self, void *closure)
{
	Safe *self = (Safe *)_self;

	return PyUnicode_DecodeFSDefault(self->safe.name);
}

static int
Safe_name_setter(PyObject *_self, PyObject *opath, void *closure)
{
	kp_error_t ret;
	char *name;
	Safe *self = (Safe *)_self;

	name = PyBytes_AsString(opath);

	if ((ret = kp_safe_rename(&self->context->ctx, &self->safe, name))
	    != KP_SUCCESS) {
		raise_kp_exception(ret);
		return -1;
	}

	return 0;
}

static PyMethodDef Safe_methods[] = {
	{"open", (PyCFunction)Safe_open, METH_VARARGS | METH_KEYWORDS, "Open safe"},
	{"close", (PyCFunction)Safe_close, METH_NOARGS, "Close safe"},
	{"save", (PyCFunction)Safe_save, METH_NOARGS, "Save safe"},
	{"delete", (PyCFunction)Safe_delete, METH_NOARGS, "Delete safe"},
	{NULL}
};

static PyMemberDef Safe_members[] = {
	{"context", T_OBJECT_EX, offsetof(Safe, context), 0, "context"},
	{NULL}
};

static PyGetSetDef Safe_getset[] = {
	{"password", Safe_password_getter, Safe_password_setter, NULL, NULL},
	{"metadata", Safe_metadata_getter, Safe_metadata_setter, NULL, NULL},
	{"path", Safe_path_getter, NULL, NULL, NULL},
	{"name", Safe_name_getter, Safe_name_setter, NULL, NULL},
	{NULL}
};

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
	Safe_getset,                 /* tp_getset */
	0,                           /* tp_base */
	0,                           /* tp_dict */
	0,                           /* tp_descr_get */
	0,                           /* tp_descr_set */
	0,                           /* tp_dictoffset */
	(initproc)Safe__init__,      /* tp_init */
	0,                           /* tp_alloc */
	Safe_new,                    /* tp_new */
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

	exception = PyErr_NewException("kickpass.Exception", NULL, NULL);
	PyModule_AddObject(m, "Exception", exception);
	PyModule_AddObject(m, "Context", (PyObject *)&ContextType);
	PyModule_AddObject(m, "Safe", (PyObject *)&SafeType);

	return m;
}

static kp_error_t
prompt_wrapper(struct kp_ctx *ctx, bool confirm, char *password, const char *fmt, va_list ap)
{
	PyObject *opassword;
	Context *py_ctx;
	char *prompt;
	char *py_password;

	size_t offset = offsetof(Context, ctx);
	py_ctx = (Context *)((void *)ctx - offset);

	if (vasprintf(&prompt, fmt, ap) < 0) {
		return KP_ERRNO;
	}

	opassword = PyObject_CallFunction(py_ctx->prompt, "OOs", py_ctx, confirm?Py_True:Py_False, prompt);
	if (opassword == NULL) {
		return KP_PYTHON_ERR;
	}

	py_password = PyBytes_AsString(opassword);
	if (py_password == NULL) {
		return KP_PYTHON_ERR;
	}

	if (strlcpy(password, py_password, KP_PASSWORD_MAX_LEN) >= KP_PASSWORD_MAX_LEN) {
		errno = ENOMEM;
		return KP_ERRNO;
	}

	return KP_SUCCESS;
}

static void
raise_kp_exception(kp_error_t err)
{
	if (err == KP_PYTHON_ERR) {
		return;
	}

	if (err != KP_ERRNO) {
		PyErr_SetObject(exception, PyUnicode_FromString(kp_strerror(err)));
	} else {
		PyErr_SetObject(exception, PyUnicode_FromString(strerror(errno)));
	}
}

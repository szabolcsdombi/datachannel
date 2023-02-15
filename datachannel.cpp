#include <Python.h>
#include <structmember.h>

#include "rtc/rtc.h"

#ifdef _WIN32
#include <Windows.h>
typedef HANDLE Event;
#define create_event() CreateEvent(NULL, false, false, NULL)
#define set_event(event) SetEvent(event)
#define wait_event(event) WaitForSingleObject(event, INFINITE)
#else
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
typedef sem_t * Event;
Event create_event() {Event event = new sem_t; sem_init(event, 0, 0); return event;}
#define set_event(event) sem_post(event)
#define wait_event(event) sem_wait(event)
#endif

struct Peer {
    PyObject_HEAD
    int pc;
    int dc;
    Event sdp_ready;
    Event dc_ready;
};

PyTypeObject * Peer_type;

void local_description_callback(int pc, const char * sdp, const char * type, void * ptr) {
    Peer * self = (Peer *)ptr;
    set_event(self->sdp_ready);
}

void data_channel_callback(int pc, int dc, void * ptr) {
    Peer * self = (Peer *)ptr;
    self->dc = dc;
    set_event(self->dc_ready);
}

void open_callback(int dc, void * ptr) {
    Peer * self = (Peer *)ptr;
    set_event(self->dc_ready);
}

PyObject * meth_logger(PyObject * self, PyObject * args, PyObject * kwargs) {
    const char * keywords[] = {"level", NULL};

    int level = RTC_LOG_INFO;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|i", (char **)keywords, &level)) {
        return NULL;
    }

    rtcInitLogger((rtcLogLevel)level, NULL);
    Py_RETURN_NONE;
}

Peer * meth_peer(PyObject * self, PyObject * args, PyObject * kwargs) {
    const char * keywords[] = {"sdp", NULL};

    PyObject * sdp = Py_None;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|O", (char **)keywords, &sdp)) {
        return NULL;
    }

    if (sdp != Py_None && !PyBytes_CheckExact(sdp)) {
        return NULL;
    }

    Peer * res = PyObject_New(Peer, Peer_type);
    res->pc = 0;
    res->dc = 0;
    res->sdp_ready = create_event();
    res->dc_ready = create_event();

    rtcConfiguration config = {};
    res->pc = rtcCreatePeerConnection(&config);
    rtcSetUserPointer(res->pc, res);
    rtcSetLocalDescriptionCallback(res->pc, local_description_callback);

    if (sdp == Py_None) {
        res->dc = rtcCreateDataChannel(res->pc, "data");
        rtcSetOpenCallback(res->dc, open_callback);
    } else {
        rtcSetDataChannelCallback(res->pc, data_channel_callback);
        rtcSetRemoteDescription(res->pc, PyBytes_AsString(sdp), "offer");
    }

    wait_event(res->sdp_ready);
    return res;
}

PyObject * Peer_meth_sdp(Peer * self) {
    int size = rtcGetLocalDescription(self->pc, NULL, 0);
    PyObject * res = PyBytes_FromStringAndSize(NULL, size);
    rtcGetLocalDescription(self->pc, PyBytes_AsString(res), size);
    return res;
}

PyObject * Peer_meth_connect(Peer * self, PyObject * args, PyObject * kwargs) {
    const char * keywords[] = {"sdp", NULL};

    PyObject * sdp;

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "S", (char **)keywords, &sdp)) {
        return NULL;
    }

    rtcSetRemoteDescription(self->pc, PyBytes_AsString(sdp), "answer");
    Py_RETURN_NONE;
}

PyObject * Peer_meth_wait(Peer * self) {
    wait_event(self->dc_ready);
    Py_RETURN_NONE;
}

PyObject * Peer_meth_buffered(Peer * self) {
    return PyLong_FromLong(rtcGetBufferedAmount(self->dc));
}

PyObject * Peer_meth_recv(Peer * self) {
    int size = 0;
    if (rtcReceiveMessage(self->dc, NULL, &size) < 0) {
        Py_RETURN_NONE;
    }
    PyObject * res = PyBytes_FromStringAndSize(NULL, size < 0 ? -size - 1 : size);
    rtcReceiveMessage(self->dc, PyBytes_AsString(res), &size);
    return res;
}

PyObject * Peer_meth_send(Peer * self, PyObject * arg) {
    if (!PyBytes_CheckExact(arg)) {
        return NULL;
    }
    rtcSendMessage(self->dc, PyBytes_AsString(arg), (int)PyBytes_Size(arg));
    Py_RETURN_NONE;
}

void default_dealloc(PyObject * self) {
    PyObject_Del(self);
}

PyMethodDef Peer_methods[] = {
    {"sdp", (PyCFunction)Peer_meth_sdp, METH_NOARGS},
    {"connect", (PyCFunction)Peer_meth_connect, METH_VARARGS | METH_KEYWORDS},
    {"wait", (PyCFunction)Peer_meth_wait, METH_NOARGS},
    {"buffered", (PyCFunction)Peer_meth_buffered, METH_NOARGS},
    {"recv", (PyCFunction)Peer_meth_recv, METH_NOARGS},
    {"send", (PyCFunction)Peer_meth_send, METH_O},
    {},
};

PyType_Slot Peer_slots[] = {
    {Py_tp_methods, Peer_methods},
    {Py_tp_dealloc, (void *)default_dealloc},
    {},
};

PyType_Spec Peer_spec = {"datachannel.Peer", sizeof(Peer), 0, Py_TPFLAGS_DEFAULT, Peer_slots};

PyMethodDef module_methods[] = {
    {"logger", (PyCFunction)meth_logger, METH_VARARGS | METH_KEYWORDS},
    {"peer", (PyCFunction)meth_peer, METH_VARARGS | METH_KEYWORDS},
    {},
};

PyModuleDef module_def = {PyModuleDef_HEAD_INIT, "datachannel", NULL, -1, module_methods};

extern "C" PyObject * PyInit_datachannel() {
    rtcPreload();
    PyObject * module = PyModule_Create(&module_def);
    Peer_type = (PyTypeObject *)PyType_FromSpec(&Peer_spec);
    return module;
}

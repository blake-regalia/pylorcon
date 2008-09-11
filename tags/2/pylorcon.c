/************************************************************
 * pylorcon.c - Python bindings for the LORCON library
 * By: Tom Wambold
 *
 * See http://802.11ninja.net/lorcon for more info on LORCON
 ***********************************************************/

#include <Python.h>
#include <structmember.h>
#include <tx80211.h>
#include <tx80211_packet.h>

static PyObject *LorconError;  // Custom exception type

/* Helper function to make a python list out of the capabilites of a card */
static PyObject* capToList(int cap) {
	PyObject *capList;
	capList = PyList_New(0);

	if ((cap & TX80211_CAP_SNIFF) != 0) 
		PyList_Append(capList, PyString_FromString("SNIFF"));
	
	if ((cap & TX80211_CAP_TRANSMIT) != 0)
		PyList_Append(capList, PyString_FromString("TRANSMIT"));
	
	if ((cap & TX80211_CAP_SEQ) != 0)
		PyList_Append(capList, PyString_FromString("SEQ"));
	
	if ((cap & TX80211_CAP_BSSTIME) != 0)
		PyList_Append(capList, PyString_FromString("BSSTIME"));
	
	if ((cap & TX80211_CAP_FRAG) != 0)
		PyList_Append(capList, PyString_FromString("FRAG"));
	
	if ((cap & TX80211_CAP_CTRL) != 0)
		PyList_Append(capList, PyString_FromString("CTRL"));
	
	if ((cap & TX80211_CAP_DURID) != 0)
		PyList_Append(capList, PyString_FromString("DURID"));
	
	if ((cap & TX80211_CAP_SNIFFACK) != 0)
		PyList_Append(capList, PyString_FromString("SNIFFACK"));
	
	if ((cap & TX80211_CAP_SELFACK) != 0)
		PyList_Append(capList, PyString_FromString("SELFACK"));
	
	if ((cap & TX80211_CAP_TXNOWAIT) != 0)
		PyList_Append(capList, PyString_FromString("TXNOWAIT"));
	
	if ((cap & TX80211_CAP_DSSSTX) != 0)
		PyList_Append(capList, PyString_FromString("DSSSTX"));
	
	if ((cap & TX80211_CAP_OFDMTX) != 0)
		PyList_Append(capList, PyString_FromString("OFDMTX"));
	
	if ((cap & TX80211_CAP_MIMOTX) != 0)
		PyList_Append(capList, PyString_FromString("MIMOTX"));
	
	if ((cap & TX80211_CAP_SETRATE) != 0)
		PyList_Append(capList, PyString_FromString("SETRATE"));
	
	if ((cap & TX80211_CAP_SETMODULATION) != 0)
		PyList_Append(capList, PyString_FromString("SETMODULATION"));
	
	if ((cap & TX80211_CAP_NONE) != 0)
		PyList_Append(capList, PyString_FromString("NONE"));

	return capList;
}

/* Args: None
 * Returns: An integer corresponding to the current Lorcon version
 */
static PyObject* pylorcon_getversion(PyObject *self, PyObject *args) {
	return Py_BuildValue("i", tx80211_getversion());
}

/* Args: None
 * Returns: A list of dictionaries containing compatible card information
 */
static PyObject* pylorcon_getcardlist(PyObject *self, PyObject *args) {
	struct tx80211_cardlist *cl;
	int c;
	PyObject *pylist;

	if (!(cl = tx80211_getcardlist())) {
		Py_INCREF(Py_None);
		return Py_None;
	}

	if (!(pylist = PyList_New(cl->num_cards))) {
		PyErr_SetString(LorconError, "Could not create a new list");
		return NULL;
	}

	for (c = 0; c < cl->num_cards; c++) {
		PyObject *pydict = PyDict_New();
		if (!pydict) {
			PyErr_SetString(LorconError, "Could not create a new list");
			return NULL;
		}
		PyDict_SetItemString(pydict, "name", 
				Py_BuildValue("s", cl->cardnames[c]));
		PyDict_SetItemString(pydict, "description", 
				Py_BuildValue("s", cl->descriptions[c]));
		PyDict_SetItemString(pydict, "capabilities", 
				Py_BuildValue("O", capToList(cl->capabilities[c])));
		PyList_SetItem(pylist, c, pydict);
	}

	tx80211_freecardlist(cl);
	return pylist;
}   

/* Lorcon object members */
typedef struct {
	// This DOES NOT need a comma!
	PyObject_HEAD
	struct tx80211 in_tx;   /* lorcon interface structure */
	struct tx80211_packet in_packet; /* packet stucture */
	PyObject *iface;            /* current interface */
	PyObject *driver;           /* current driver */
	int drivertype;         /* numerical driver value */
} Lorcon;

/* This gets called when the object is cleaned up */
static void Lorcon_dealloc(Lorcon* self) {
	if (tx80211_getmode(&self->in_tx) >= 0) {
		tx80211_close(&self->in_tx);
	}
	self->ob_type->tp_free((PyObject*)self);
}

/* This will get called when this or any object decending from this object is
   created */
static PyObject* Lorcon_new(PyTypeObject *type, PyObject *ards, PyObject *kwds) {

	Lorcon *self;
	self = (Lorcon *)type->tp_alloc(type, 0);

	if (self != NULL) {
		self->drivertype = INJ_NODRIVER;
	}
	return (PyObject *)self;
}

/* This is equivalent to the __init__ python function.
 * Args: iface - String with the wireless interface to use
 *       driver - The Lorcon driver to use
 */
static int Lorcon_init(Lorcon *self, PyObject *args, PyObject *kwds) {
	PyObject *iface, *driver;
	static char *kwlist[] = {"iface", "driver", NULL};

	if (!PyArg_ParseTupleAndKeywords(args, kwds, "SS", kwlist,
				&iface, &driver)) {
		return -1;
	}

	// Keep refrences to our arguments
	Py_INCREF(iface);
	self->iface = iface;
	Py_INCREF(driver);
	self->driver = driver;

	self->drivertype = tx80211_resolvecard(PyString_AsString(self->driver));

	if (self->drivertype == INJ_NODRIVER) {
		PyErr_SetString(LorconError, "Invalid driver specified.");
		return -1;
	}

	if (tx80211_init(&self->in_tx, PyString_AsString(self->iface),
				self->drivertype) < 0) {
		PyErr_SetString(LorconError, tx80211_geterrstr(&self->in_tx));
		return -1;
	}

	if (tx80211_open(&self->in_tx) < 0) {
		PyErr_SetString(LorconError, tx80211_geterrstr(&self->in_tx));
		return -1;
	}
	tx80211_initpacket(&self->in_packet);

	return 0;
}

/*** BEGIN Python Lorcon object methods ***/

/* Returns the currently used interface */
static PyObject* Lorcon_getiface(Lorcon *self) {
	return self->iface;
}

/* Returns the currently used driver */
static PyObject* Lorcon_getdriver(Lorcon *self) {
	return self->driver;
}

/* Wraps the tx80211_getmode function */
static PyObject* Lorcon_getmode(Lorcon* self) {
	int mode;

	mode = tx80211_getmode(&self->in_tx);
	if (mode < 0) {
		PyErr_SetString(LorconError, tx80211_geterrstr(&self->in_tx));
		return NULL;
	}
	switch (mode) {
		case TX80211_MODE_AUTO:
			return PyString_FromString("AUTO");
			break;
		case TX80211_MODE_ADHOC:
			return PyString_FromString("ADHOC");
			break;
		case TX80211_MODE_INFRA:
			return PyString_FromString("INFRA");
			break;
		case TX80211_MODE_MASTER:
			return PyString_FromString("MASTER");
			break;
		case TX80211_MODE_REPEAT:
			return PyString_FromString("REPEAT");
			break;
		case TX80211_MODE_SECOND:
			return PyString_FromString("SECOND");
			break;
		case TX80211_MODE_MONITOR:
			return PyString_FromString("MONITOR");
			break;
		default:
			return NULL;
			break;
	}
}

/* Wraps the tx80211_setmode function */
static PyObject* Lorcon_setmode(Lorcon* self, PyObject *args, PyObject *kwds) {
	char *setmode;
	int mode = -1;

	if (!PyArg_ParseTuple(args, "s", &setmode)) {
		return NULL;
	}

	if (strcmp(setmode, "AUTO") == 0) {
		mode = TX80211_MODE_AUTO;
	} else if (strcmp(setmode, "ADHOC") == 0) {
		mode = TX80211_MODE_ADHOC;
	} else if (strcmp(setmode, "INFRA") == 0) {
		mode = TX80211_MODE_INFRA;
	} else if (strcmp(setmode, "MASTER") == 0) {
		mode = TX80211_MODE_MASTER;
	} else if (strcmp(setmode, "REPEAT") == 0) {
		mode = TX80211_MODE_REPEAT;
	} else if (strcmp(setmode, "SECOND") == 0) {
		mode = TX80211_MODE_SECOND;
	} else if (strcmp(setmode, "MONITOR") == 0) {
		mode = TX80211_MODE_MONITOR;
	} else {
		PyErr_SetString(LorconError, "Invalid Mode");
	}

	Py_INCREF(Py_None);
	return Py_None;
}

/* Wraps the tx80211_setfunctionalmode function */
static PyObject* Lorcon_setfunctionalmode(Lorcon* self, PyObject *args, PyObject *kwds) {
	char *funcmode;
	int mode = -1;

	if (!PyArg_ParseTuple(args, "s", &funcmode)) {
		return NULL;
	}

	if (strcmp(funcmode, "RFMON") == 0) {
		mode = TX80211_FUNCMODE_RFMON;
	} else if (strcmp(funcmode, "INJECT") == 0) {
		mode = TX80211_FUNCMODE_INJECT;
	} else if (strcmp(funcmode, "INJMON") == 0) {
		mode = TX80211_FUNCMODE_INJMON;
	} else {
		PyErr_SetString(LorconError, "Invalid mode specified");
	}

	if (tx80211_setfunctionalmode(&self->in_tx, mode) != 0) {
		PyErr_SetString(LorconError, tx80211_geterrstr(&self->in_tx));
		return NULL;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

/* Wraps the tx80211_getchannel function */
static PyObject* Lorcon_getchannel(Lorcon* self) {
	int channel;

	channel = tx80211_getchannel(&self->in_tx);
	if (channel < 0) {
		PyErr_SetString(LorconError, tx80211_geterrstr(&self->in_tx));
		return NULL;
	}

	return PyInt_FromSsize_t(channel);
}

/* Wraps the tx80211_setchannel function */
static PyObject* Lorcon_setchannel(Lorcon* self, PyObject *args, PyObject *kwds) {
	int channel;

	if (!PyArg_ParseTuple(args, "i", &channel)) {
		return NULL;
	}

	if (tx80211_setchannel(&self->in_tx, channel) == -1) {
		PyErr_SetString(LorconError, tx80211_geterrstr(&self->in_tx));
		return NULL;
	}
	
	Py_INCREF(Py_None);
	return Py_None;
}

/* Wraps the tx80211_gettxrate function */
static PyObject* Lorcon_gettxrate(Lorcon* self) {
	int txrate;
	txrate = tx80211_gettxrate(&self->in_packet);

	switch (txrate) {
		case TX80211_RATE_DEFAULT:
			return PyInt_FromSsize_t(0);
			break;
		case TX80211_RATE_1MB:
			return PyInt_FromSsize_t(1);
			break;
		case TX80211_RATE_2MB:
			return PyInt_FromSsize_t(2);
			break;
		case TX80211_RATE_5_5MB:
			return PyInt_FromSsize_t(5);
			break;
		case TX80211_RATE_6MB:
			return PyInt_FromSsize_t(6);
			break;
		case TX80211_RATE_9MB:
			return PyInt_FromSsize_t(9);
			break;
		case TX80211_RATE_11MB:
			return PyInt_FromSsize_t(11);
			break;
		case TX80211_RATE_24MB:
			return PyInt_FromSsize_t(24);
			break;
		case TX80211_RATE_36MB:
			return PyInt_FromSsize_t(36);
			break;
		case TX80211_RATE_48MB:
			return PyInt_FromSsize_t(48);
			break;
		case TX80211_RATE_108MB:
			return PyInt_FromSsize_t(108);
			break;
		default:
			PyErr_SetString(LorconError, "Error parsing current txrate");
			return NULL;
	}
}

/* Wraps the tx80211_settxrate function */
static PyObject* Lorcon_settxrate(Lorcon *self, PyObject *args, PyObject *kwds) {
	float settxrate = -1;
	int txrate = -1;

	if ((tx80211_getcapabilities(&self->in_tx) & TX80211_CAP_SETRATE) == 0) {
		PyErr_SetString(LorconError, "Card does not support setting rate");
		return NULL;
	}

	if (!PyArg_ParseTuple(args, "|f", &settxrate)) {
		return NULL;
	}

	if (settxrate == -1) {
		txrate = TX80211_RATE_DEFAULT;
	} else if (settxrate == 1) {
		txrate = TX80211_RATE_1MB;
	} else if (settxrate == 2) {
		txrate = TX80211_RATE_2MB;
	} else if (settxrate == 5.5) {
		txrate = TX80211_RATE_5_5MB;
	} else if (settxrate == 6) {
		txrate = TX80211_RATE_6MB;
	} else if (settxrate == 9) {
		txrate = TX80211_RATE_9MB;
	} else if (settxrate == 11) {
		txrate = TX80211_RATE_11MB;
	} else if (settxrate == 24) {
		txrate = TX80211_RATE_24MB;
	} else if (settxrate == 36) {
		txrate = TX80211_RATE_36MB;
	} else if (settxrate == 48) {
		txrate = TX80211_RATE_48MB;
	} else if (settxrate == 108) {
		txrate = TX80211_RATE_108MB;
	} else {
		PyErr_SetString(LorconError, "Invalid Rate");
		return NULL;
	}

	if (tx80211_settxrate(&self->in_tx, &self->in_packet, txrate) < 0) {
		PyErr_SetString(LorconError, tx80211_geterrstr(&self->in_tx));
		return NULL;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

/* Wraps the tx80211_getmodulation funciton */
static PyObject* Lorcon_getmodulation(Lorcon *self) {
	int mod;

	mod = tx80211_getmodulation(&self->in_packet);
	switch (mod) {
		case TX80211_MOD_DEFAULT:
			return PyString_FromString("DEFAULT");
			break;
		case TX80211_MOD_FHSS:
			return PyString_FromString("FHSS");
			break;
		case TX80211_MOD_DSSS:
			return PyString_FromString("DSSS");
			break;
		case TX80211_MOD_OFDM:
			return PyString_FromString("OFDM");
			break;
		case TX80211_MOD_TURBO:
			return PyString_FromString("TURBO");
			break;
		case TX80211_MOD_MIMO:
			return PyString_FromString("MIMO");
			break;
		case TX80211_MOD_MIMOGF:
			return PyString_FromString("MIMOGF");
			break;
		default:
			PyErr_SetString(LorconError, "Could not get mode.");
			return NULL;
	}
}

/* Wraps the tx80211_setmodulation function */
static PyObject* Lorcon_setmodulation(Lorcon *self, PyObject *args, PyObject *kwds) {
	char *setmod = NULL;
	int mod;

	if ((tx80211_getcapabilities(&self->in_tx) & TX80211_CAP_SETMODULATION) == 0) {
		PyErr_SetString(LorconError, "Card does not support setting modulation");
		return NULL;
	}

	if (!PyArg_ParseTuple(args, "s", &setmod)) {
		return NULL;
	}

	if (strcmp(setmod, "DEFAULT") == 0) {
		mod = TX80211_MOD_DEFAULT;
	} else if (strcmp(setmod, "FHSS") == 0) {
		mod = TX80211_MOD_FHSS;
	} else if (strcmp(setmod, "DSSS") == 0) {
		mod = TX80211_MOD_DSSS;
	} else if (strcmp(setmod, "OFDM") == 0) {
		mod = TX80211_MOD_OFDM;
	} else if (strcmp(setmod, "TURBO") == 0) {
		mod = TX80211_MOD_TURBO;
	} else if (strcmp(setmod, "MIMO") == 0) {
		mod = TX80211_MOD_MIMO;
	} else if (strcmp(setmod, "MIMOGF") == 0) {
		mod = TX80211_MOD_MIMOGF;
	} else {
		PyErr_SetString(LorconError, "Invalid modulation setting");
		return NULL;
	}

	if (tx80211_setmodulation(&self->in_tx, &self->in_packet, mod) < 0) {
		PyErr_SetString(LorconError, tx80211_geterrstr(&self->in_tx));
		return NULL;
	}
	Py_INCREF(Py_None);
	return Py_None;
}

/* Wraps the tx80211_getcapabilities function */
static PyObject* Lorcon_getcapabilities(Lorcon *self) {
	return capToList(tx80211_getcapabilities(&self->in_tx));
}
/*** END Python Lorcon object methods ***/

/* Wraps the tx80211_txpacket function */
static PyObject* Lorcon_txpacket(Lorcon *self, PyObject *args, PyObject *kwds) {
	unsigned char *packet;
	int length;

	if (!PyArg_ParseTuple(args, "s#", &packet, &length)) {
		return NULL;
	}

	self->in_packet.packet = packet;
	self->in_packet.plen = length;
//	self->in_packet.plen = sizeof(packet);

	if (tx80211_txpacket(&self->in_tx, &self->in_packet) < 0) {
		PyErr_SetString(LorconError, tx80211_geterrstr(&self->in_tx));
		return NULL;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyMemberDef Lorcon_members[] = {
	{NULL}  /* Sentinel */
};

static PyMethodDef Lorcon_methods[] = {
	{"getiface", (PyCFunction)Lorcon_getiface, METH_NOARGS,
		"Args: None\nReturns: The currently set wireless interface."},
	{"getdriver", (PyCFunction)Lorcon_getdriver, METH_NOARGS,
		"Args: None\nReturns: The currently set Lorcon driver."},
	{"getmode", (PyCFunction)Lorcon_getmode, METH_NOARGS,
		"Args: None\nReturns: A string representing the current mode."},
	{"setmode", (PyCFunction)Lorcon_setmode, METH_VARARGS,
		"Args: String ('AUTO', 'ADHOC', 'INFRA', 'MASTER', 'REPEAT', 'SECOND'\
			'MONITOR')\nReturns: None"},
	{"setfunctionalmode", (PyCFunction)Lorcon_setfunctionalmode, METH_VARARGS,
		"Args: String ('RFMON', 'INJECT', 'INJMON')\nReturns: None."},
	{"getchannel", (PyCFunction)Lorcon_getchannel, METH_NOARGS,
		"Args: None\nReturns: The currently set channel."},
	{"setchannel", (PyCFunction)Lorcon_setchannel, METH_VARARGS,
		"Args: Channel Int\nReturns: None"},
	{"gettxrate", (PyCFunction)Lorcon_gettxrate, METH_NOARGS,
		"Args: None\nReturns: The currently set rate in Mbs.  A value of 0\
			means that the driver is set to the default rate."},
	{"settxrate", (PyCFunction)Lorcon_settxrate, METH_VARARGS,
		"Args: rate - integer \
			(optional - 1,2,5.5,6,9,11,22,24,36,48,72,96,108)\
			Leave blank to set to default rate.\nReturns: None"},
	{"getmodulation", (PyCFunction)Lorcon_getmodulation, METH_NOARGS,
		"Args: None\nReturns: String representing the current modulation mode"},
	{"setmodulation", (PyCFunction)Lorcon_setmodulation, METH_VARARGS,
		"Args: String (DEFAULT, FHSS, DSSS, OFDM, TURBO, MIMO, MIMOGF)\n\
			Returns: None"},
	{"getcapabilities", (PyCFunction)Lorcon_getcapabilities, METH_NOARGS,
		"Args: None\nReturns: List containing capabilitity information"},
	{"txpacket", (PyCFunction)Lorcon_txpacket, METH_VARARGS,
		"Args: String to send\nReturns: None"},
	{NULL}
};

static PyTypeObject LorconType = {
	PyObject_HEAD_INIT(NULL)
	0,                         /*ob_size*/
	"pylorcon.Lorcon",             /*tp_name*/
	sizeof(Lorcon), /*tp_basicsize*/
	0,                         /*tp_itemsize*/
	(destructor)Lorcon_dealloc, /*tp_dealloc*/
	0,                         /*tp_print*/
	0,                         /*tp_getattr*/
	0,                         /*tp_setattr*/
	0,                         /*tp_compare*/
	0,                         /*tp_repr*/
	0,                         /*tp_as_number*/
	0,                         /*tp_as_sequence*/
	0,                         /*tp_as_mapping*/
	0,                         /*tp_hash */
	0,                         /*tp_call*/
	0,                         /*tp_str*/
	0,                         /*tp_getattro*/
	0,                         /*tp_setattro*/
	0,                         /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
	"Base Lorcon object",      /* tp_doc */
	0,                         /* tp_traverse */
	0,                         /* tp_clear */
	0,                         /* tp_richcompare */
	0,                         /* tp_weaklistoffset */
	0,                         /* tp_iter */
	0,                         /* tp_iternext */
	Lorcon_methods,            /* tp_methods */
	Lorcon_members,            /* tp_members */
	0,                         /* tp_getset */
	0,                         /* tp_base */
	0,                         /* tp_dict */
	0,                         /* tp_descr_get */
	0,                         /* tp_descr_set */
	0,                         /* tp_dictoffset */
	(initproc)Lorcon_init,     /* tp_init */
	0,                         /* tp_alloc */
	Lorcon_new                 /* tp_new */
};

static PyMethodDef pylorcon_methods[] = {
	{"version", pylorcon_getversion, METH_VARARGS,
		"Returns the LORCON internal version in the format YYYYMMRR (year-month-release#)."},
	{"getcardlist", pylorcon_getcardlist, METH_VARARGS,
		"Returns a list representing the list of supported wireless cards or None, if there's an error."},
	{NULL, NULL, 0, NULL}
};

#ifndef PyMODINIT_FUNC /* declarations for DLL import/exprt */
#define PyMODINIT_FUNC void
#endif

PyMODINIT_FUNC initpylorcon(void) {
	PyObject* m;

	if (PyType_Ready(&LorconType) < 0)
		return;

	m = Py_InitModule3("pylorcon", pylorcon_methods,
			"Python LORCON wrapper.");

	if (m == NULL)
		return;

	LorconError = PyErr_NewException("pylorcon.LorconError", NULL, NULL);
	Py_INCREF(LorconError);
	PyModule_AddObject(m, "LorconError", LorconError);

	Py_INCREF(&LorconType);
	PyModule_AddObject(m, "Lorcon", (PyObject *)&LorconType);
}

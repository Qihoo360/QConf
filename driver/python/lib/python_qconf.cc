#include <Python.h>
#include "qconf.h"
static PyObject* QconfError;

#define QCONF_DRIVER_PYTHON_VERSION  "1.2.2"
#if PY_MAJOR_VERSION >= 3
    #define Pys_FromString(val) PyBytes_FromString(val)
#else
    #define Pys_FromString(val) PyString_FromString(val)
#endif

static int print_error_message(const int error_code)
{
    const char* message = NULL;
    switch (error_code)
    {
        case QCONF_ERR_OTHER :
            message = "Execute failure!";
            break;
        case QCONF_ERR_PARAM :
            message = "Error parameter!";
            break;
        case QCONF_ERR_MEM :
            message = "Failed to malloc memory!";
            break;
        case QCONF_ERR_TBL_SET :
            message = "Failed to set share memory";
            break;
        case QCONF_ERR_GET_HOST :
            message = "Failed to get zookeeper host!";
            break;
        case QCONF_ERR_GET_IDC :
            message = "Failed to get idc!";
            break;
        case QCONF_ERR_BUF_NOT_ENOUGH :
            message = "Buffer not enough!";
            break;
        case QCONF_ERR_DATA_TYPE :
            message = "Illegal data type!";
            break;
        case QCONF_ERR_DATA_FORMAT :
            message = "Illegal data format!";
            break;
        case QCONF_ERR_NOT_FOUND :
            message = "Failed to find the key on given idc!";
            break;
        case QCONF_ERR_OPEN_DUMP :
            message = "Failed to open dump file!";
            break;
        case QCONF_ERR_OPEN_TMP_DUMP :
            message = "Failed to open tmp dump file!";
            break;
        case QCONF_ERR_NOT_IN_DUMP :
            message = "Failed to find key in dump!";
            break;
        case QCONF_ERR_WRITE_DUMP :
            message = "Failed to write dump!";
            break;
        case QCONF_ERR_SAME_VALUE :
            message = "Same with the value in share memory!";
            break;
        case QCONF_ERR_OUT_OF_RANGE :
            message = "Configure item error : out of range!";
            break;
        case QCONF_ERR_NOT_NUMBER :
            message = "Configure item error : not number!";
            break;
        case QCONF_ERR_OTHRE_CHARACTER :
            message = "Configure item error : further characters exists!";
            break;
        case QCONF_ERR_INVALID_IP :
            message = "Configure item error : invalid ip!";
            break;
        case QCONF_ERR_INVALID_PORT :
            message = "Configure item error : invalid port!";
            break;
        case QCONF_ERR_NO_MESSAGE :
            message = "No message exist in message queue!";
            break;
        case QCONF_ERR_E2BIG :
            message = "Length of message in the queue is too large!";
            break;
        case QCONF_ERR_HOSTNAME :
            message = "Error hostname!";
            break;
        default :
            message = "Unknown error detected!";
            break;
    }
    PyErr_SetString(QconfError, message);
    return QCONF_OK;
}


static void Qconf_dealloc(void) 
{
    // Destroy the qconf env
    int ret = qconf_destroy();
    if (QCONF_OK != ret)
    {
        PyErr_SetString(QconfError, "destory qconf evn failed!");
        return;
    }
}

static int Qconf_init(void) 
{
    // Init the qconf env
    int ret = qconf_init();
    if (QCONF_OK != ret)
    {
        PyErr_SetString(QconfError, "inital qconf evn failed!");
        return QCONF_ERR_OTHER;
    }
    return QCONF_OK;
}

static PyObject* Qconf_get_conf(PyObject *self, PyObject *args)
{
    char* path = NULL;
    char* idc = NULL;
    if (!PyArg_ParseTuple(args, "s|s", &path, &idc))
        return NULL;
    char value[QCONF_CONF_BUF_MAX_LEN];
    int ret = qconf_get_conf(path, value, sizeof(value), idc);
    if (QCONF_OK != ret)
    {
        print_error_message(ret);
        return NULL;
    }

    return Pys_FromString(value);
}

static PyObject* Qconf_get_host(PyObject *self, PyObject *args)
{
    char* path = NULL;
    char* idc = NULL;
    if (!PyArg_ParseTuple(args, "s|s", &path, &idc))
        return NULL;
    char host[QCONF_HOST_BUF_MAX_LEN] = {0};
    int ret = qconf_get_host(path, host, sizeof(host), idc);
    if (QCONF_OK != ret)
    {
        print_error_message(ret);
        return NULL;
    }

    return Pys_FromString(host);
}

static PyObject* convert_to_pylist(const string_vector_t *nodes)
{
    PyObject *item = NULL;
    PyObject *pyList = NULL;
    int i = 0;
    if (NULL == nodes)
    {
        return NULL;
    }
    int count = (*nodes).count;
    pyList = PyList_New(count);
    if (NULL == pyList)
    {
        return NULL;
    }
    for (i = 0; i < count; i++){
        item = Pys_FromString((*nodes).data[i]);
        PyList_SetItem(pyList, i, item);
    }
    return pyList;
}

static PyObject* convert_to_pydict(const qconf_batch_nodes *bnodes)
{
    PyObject *pyDict = NULL;
    int i = 0;
    if (NULL == bnodes)
    {
        return NULL;
    }
    int count = (*bnodes).count;
    pyDict = PyDict_New();
    if (NULL == pyDict)
    {
        return NULL;
    }
    PyObject *pval = NULL;
    for (i = 0; i < count; i++)
    {
        pval =  Pys_FromString((*bnodes).nodes[i].value);
        if (NULL != pval)
        {
            PyDict_SetItemString(pyDict, (*bnodes).nodes[i].key, pval);
            Py_DECREF(pval);
        }
    }
    return pyDict;
}

static PyObject* Qconf_get_allhost(PyObject *self, PyObject *args)
{
    int ret = QCONF_OK;
    char* path = NULL;
    char* idc = NULL;
    if (!PyArg_ParseTuple(args, "s|s", &path, &idc))
        return NULL;

    string_vector_t nodes;
    ret = init_string_vector(&nodes);
    if (QCONF_OK != ret)
    {
        print_error_message(ret);
        return NULL;
    }
    ret = qconf_get_allhost(path, &nodes, idc);
    if (QCONF_OK != ret)
    {
        destroy_string_vector(&nodes);
        print_error_message(ret);
        return NULL;
    }
    PyObject* result = convert_to_pylist(&nodes);
    destroy_string_vector(&nodes);
    if (NULL == result)
    {
        PyErr_SetString(QconfError, "Failed to convert to python list!");
        return NULL;
    }
    return result;
}

static PyObject* Qconf_get_batch_conf(PyObject *self, PyObject *args)
{
    int ret = QCONF_OK;
    char* path = NULL;
    char* idc = NULL;
    if (!PyArg_ParseTuple(args, "s|s", &path, &idc))
        return NULL;

    qconf_batch_nodes bnodes;
    ret = init_qconf_batch_nodes(&bnodes);
    if (QCONF_OK != ret)
    {
        print_error_message(ret);
        return NULL;
    }
    ret = qconf_get_batch_conf(path, &bnodes, idc);
    if (QCONF_OK != ret)
    {
        destroy_qconf_batch_nodes(&bnodes);
        print_error_message(ret);
        return NULL;
    }

    PyObject* result = convert_to_pydict(&bnodes);
    destroy_qconf_batch_nodes(&bnodes);
    if (NULL == result)
    {
        PyErr_SetString(QconfError, "Failed to convert to python dictionary!");
        return NULL;
    }
    return result;
}

static PyObject* Qconf_get_batch_keys(PyObject *self, PyObject *args)
{
    int ret = QCONF_OK;
    char* path = NULL;
    char* idc = NULL;
    if (!PyArg_ParseTuple(args, "s|s", &path, &idc))
        return NULL;

    string_vector_t bnodes_key;
    ret = init_string_vector(&bnodes_key);
    if (QCONF_OK != ret)
    {
        print_error_message(ret);
        return NULL;
    }
    ret = qconf_get_batch_keys(path, &bnodes_key, idc);
    if (QCONF_OK != ret)
    {
        destroy_string_vector(&bnodes_key);
        print_error_message(ret);
        return NULL;
    }
    PyObject* result = convert_to_pylist(&bnodes_key);
    destroy_string_vector(&bnodes_key);
    if (NULL == result)
    {
        PyErr_SetString(QconfError, "Failed to convert batch keys to python list!");
        return NULL;
    }
    return result;
}

static PyObject* Qconf_version(PyObject *self)
{
    return Pys_FromString(QCONF_DRIVER_PYTHON_VERSION);
}


static PyMethodDef qconf_methods[] = {
    {"get_conf", (PyCFunction)Qconf_get_conf, METH_VARARGS, "get conf value"},
    {"get_host", (PyCFunction)Qconf_get_host, METH_VARARGS, "get one service host"},
    {"get_allhost", (PyCFunction)Qconf_get_allhost, METH_VARARGS, "get all service hosts"},
    {"get_batch_conf", (PyCFunction)Qconf_get_batch_conf, METH_VARARGS, "get all children nodes"},
    {"get_batch_keys", (PyCFunction)Qconf_get_batch_keys, METH_VARARGS, "get key of all children nodes"},
    {"version", (PyCFunction)Qconf_version, METH_NOARGS, "qconf version"},
    {NULL, NULL, 0, NULL}
};

#if PY_MAJOR_VERSION >= 3
static struct PyModuleDef moduledef = {
    PyModuleDef_HEAD_INIT,
    "qconf_py",     /* m_name */
    "Python extension of Qconf",  /* m_doc */
    -1,                  /* m_size */
    qconf_methods,    /* m_methods */
    NULL,                /* m_reload */
    NULL,                /* m_traverse */
    NULL,                /* m_clear */
    NULL,                /* m_free */
};
#endif

#if PY_MAJOR_VERSION >= 3
    #define MOD_INIT(name) PyMODINIT_FUNC PyInit_##name(void)
#else
    #define MOD_INIT(name) PyMODINIT_FUNC init##name(void)
#endif


MOD_INIT(qconf_py)
{
    PyObject *m;
#if PY_MAJOR_VERSION >= 3
    m = PyModule_Create(&moduledef);
#else
    m = Py_InitModule3("qconf_py", qconf_methods, "Python extension of Qconf");
#endif
    if (NULL == m)
#if PY_MAJOR_VERSION >= 3
        return NULL;
#else
        return;
#endif
    QconfError = PyErr_NewException("qconf_py.Error", NULL, NULL);
    Py_INCREF(QconfError);
    PyModule_AddObject(m, "Error", QconfError);

    int ret = Qconf_init();
    if (QCONF_ERR_OTHER == ret)
#if PY_MAJOR_VERSION >= 3
        return NULL;
#else
        return;
#endif
    ret = Py_AtExit(Qconf_dealloc);
    if (-1 == ret)
    {
        Qconf_dealloc();
#if PY_MAJOR_VERSION >= 3
        return NULL;
#else
        return;
#endif
    }
#if PY_MAJOR_VERSION >= 3
    return m;
#else
    return;
#endif
}

#include "PythonEnv.h"
#include "spot/utility/Logging.h"
#include "spot/utility/MeasureFunc.h"
#include <Python.h>

using namespace spot::utility;
using namespace spot::base;

PythonEnv::PythonEnv()
{
	isInit_ = false;
}

PythonEnv::~PythonEnv()
{
}

PythonEnv& PythonEnv::instance()
{
	static PythonEnv instance;
	return instance;
}

void PythonEnv::init()
{
	if (isInit_)
	{
		return;
	}

	Py_Initialize();
	if (Py_IsInitialized() == 0)
	{
		LOG_FATAL << "Py_Initialize failed";
		return;
	}
	LOG_INFO << "Python version:" << Py_GetVersion();

	PyRun_SimpleString("import sys");
	PyRun_SimpleString("sys.path.append('./pyUtility')");

	isInit_ = true;
}

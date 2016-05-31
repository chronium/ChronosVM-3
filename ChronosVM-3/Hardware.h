#pragma once

class VirtualMachine;
#include "VirtualMachine.h"

class Hardware {
public:
	Hardware (VirtualMachine *VM);
	virtual ~Hardware ();

	virtual void Start () { };
	virtual void Stop () { };
	virtual void Pause () { };

	inline VirtualMachine *GetVM () const { return this->VM; }
private:
	VirtualMachine *VM;
};


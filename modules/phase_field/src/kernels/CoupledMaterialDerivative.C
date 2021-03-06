//* This file is part of the MOOSE framework
//* https://www.mooseframework.org
//*
//* All rights reserved, see COPYRIGHT for full restrictions
//* https://github.com/idaholab/moose/blob/master/COPYRIGHT
//*
//* Licensed under LGPL 2.1, please see LICENSE for details
//* https://www.gnu.org/licenses/lgpl-2.1.html

#include "CoupledMaterialDerivative.h"

registerMooseObject("PhaseFieldApp", CoupledMaterialDerivative);

defineLegacyParams(CoupledMaterialDerivative);

InputParameters
CoupledMaterialDerivative::validParams()
{
  InputParameters params = Kernel::validParams();
  params.addClassDescription("Kernel that implements the first derivative of a function material "
                             "property with respect to a coupled variable.");
  params.addRequiredCoupledVar("v", "Variable to take the derivative with respect to");
  params.addParam<MaterialPropertyName>("f_name",
                                        "F",
                                        "Function material to take the derivative of (should "
                                        "provide derivative properties - such as a "
                                        "DerivativeParsedMaterial)");
  params.addCoupledVar(
      "args",
      "Vector of other nonlinear variables F depends on (used for computing Jacobian entries)");
  return params;
}

CoupledMaterialDerivative::CoupledMaterialDerivative(const InputParameters & parameters)
  : DerivativeMaterialInterface<JvarMapKernelInterface<Kernel>>(parameters),
    _v_name(getVar("v", 0)->name()),
    _v_var(coupled("v")),
    _dFdv(getMaterialPropertyDerivative<Real>("f_name", _v_name)),
    _d2Fdvdu(getMaterialPropertyDerivative<Real>("f_name", _v_name, _var.name())),
    _nvar(_coupled_moose_vars.size()),
    _d2Fdvdarg(_nvar)
{
  // Get material property derivatives for all coupled variables
  for (unsigned int i = 0; i < _nvar; ++i)
  {
    MooseVariableFEBase * ivar = _coupled_moose_vars[i];
    _d2Fdvdarg[i] = &getMaterialPropertyDerivative<Real>("f_name", _v_name, ivar->name());
  }
}

void
CoupledMaterialDerivative::initialSetup()
{
  validateNonlinearCoupling<Real>("f_name");
}

Real
CoupledMaterialDerivative::computeQpResidual()
{
  return _dFdv[_qp] * _test[_i][_qp];
}

Real
CoupledMaterialDerivative::computeQpJacobian()
{
  return _d2Fdvdu[_qp] * _test[_i][_qp] * _phi[_j][_qp];
}

Real
CoupledMaterialDerivative::computeQpOffDiagJacobian(unsigned int jvar)
{
  const unsigned int cvar = mapJvarToCvar(jvar);
  return (*_d2Fdvdarg[cvar])[_qp] * _test[_i][_qp] * _phi[_j][_qp];
}

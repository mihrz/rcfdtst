/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | Copyright (C) 2011-2012 OpenFOAM Foundation
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

\*---------------------------------------------------------------------------*/

#include "supersonicFreestreamFvPatchVectorField.H"
#include "addToRunTimeSelectionTable.H"
#include "fvPatchFieldMapper.H"
#include "volFields.H"

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::supersonicFreestreamFvPatchVectorField::
supersonicFreestreamFvPatchVectorField
(
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF
)
:
    mixedFvPatchVectorField(p, iF),
    TName_("T"),
    pName_("p"),
    psiName_("thermo:psi"),
    UInf_(vector::zero),
    pInf_(0),
    TInf_(0),
    gamma_(0)
{
    refValue() = patchInternalField();
    refGrad() = vector::zero;
    valueFraction() = 1;
}


Foam::supersonicFreestreamFvPatchVectorField::
supersonicFreestreamFvPatchVectorField
(
    const supersonicFreestreamFvPatchVectorField& ptf,
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF,
    const fvPatchFieldMapper& mapper
)
:
    mixedFvPatchVectorField(ptf, p, iF, mapper),
    TName_(ptf.TName_),
    pName_(ptf.pName_),
    psiName_(ptf.psiName_),
    UInf_(ptf.UInf_),
    pInf_(ptf.pInf_),
    TInf_(ptf.TInf_),
    gamma_(ptf.gamma_)
{}


Foam::supersonicFreestreamFvPatchVectorField::
supersonicFreestreamFvPatchVectorField
(
    const fvPatch& p,
    const DimensionedField<vector, volMesh>& iF,
    const dictionary& dict
)
:
    mixedFvPatchVectorField(p, iF),
    TName_(dict.lookupOrDefault<word>("T", "T")),
    pName_(dict.lookupOrDefault<word>("p", "p")),
    psiName_(dict.lookupOrDefault<word>("psi", "thermo:psi")),
    UInf_(dict.lookup("UInf")),
    pInf_(readScalar(dict.lookup("pInf"))),
    TInf_(readScalar(dict.lookup("TInf"))),
    gamma_(readScalar(dict.lookup("gamma")))
{
    if (dict.found("value"))
    {
        fvPatchField<vector>::operator=
        (
            vectorgpuField("value", dict, p.size())
        );
    }
    else
    {
        fvPatchField<vector>::operator=(patchInternalField());
    }

    refValue() = *this;
    refGrad() = vector::zero;
    valueFraction() = 1;

    if (pInf_ < SMALL)
    {
        FatalIOErrorIn
        (
            "supersonicFreestreamFvPatchVectorField::"
            "supersonicFreestreamFvPatchVectorField"
            "(const fvPatch&, const vectorField&, const dictionary&)",
            dict
        )   << "    unphysical pInf specified (pInf <= 0.0)"
            << "\n    on patch " << this->patch().name()
            << " of field " << this->dimensionedInternalField().name()
            << " in file " << this->dimensionedInternalField().objectPath()
            << exit(FatalIOError);
    }
}


Foam::supersonicFreestreamFvPatchVectorField::
supersonicFreestreamFvPatchVectorField
(
    const supersonicFreestreamFvPatchVectorField& sfspvf
)
:
    mixedFvPatchVectorField(sfspvf),
    TName_(sfspvf.TName_),
    pName_(sfspvf.pName_),
    psiName_(sfspvf.psiName_),
    UInf_(sfspvf.UInf_),
    pInf_(sfspvf.pInf_),
    TInf_(sfspvf.TInf_),
    gamma_(sfspvf.gamma_)
{}


Foam::supersonicFreestreamFvPatchVectorField::
supersonicFreestreamFvPatchVectorField
(
    const supersonicFreestreamFvPatchVectorField& sfspvf,
    const DimensionedField<vector, volMesh>& iF
)
:
    mixedFvPatchVectorField(sfspvf, iF),
    TName_(sfspvf.TName_),
    pName_(sfspvf.pName_),
    psiName_(sfspvf.psiName_),
    UInf_(sfspvf.UInf_),
    pInf_(sfspvf.pInf_),
    TInf_(sfspvf.TInf_),
    gamma_(sfspvf.gamma_)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::supersonicFreestreamFvPatchVectorField::updateCoeffs()
{
    if (!size() || updated())
    {
        return;
    }

    const fvPatchField<scalar>& pT =
        patch().lookupPatchField<volScalarField, scalar>(TName_);

    const fvPatchField<scalar>& pp =
        patch().lookupPatchField<volScalarField, scalar>(pName_);

    const fvPatchField<scalar>& ppsi =
        patch().lookupPatchField<volScalarField, scalar>(psiName_);

    // Need R of the free-stream flow.  Assume R is independent of location
    // along patch so use face 0
    // scalar R = 1.0/(ppsi[0]*pT[0]);
    scalar R = 1.0/(ppsi.get(0)*pT.get(0));

    scalar MachInf = mag(UInf_)/sqrt(gamma_*R*TInf_);

    if (MachInf < 1.0)
    {
        FatalErrorIn
        (
            "supersonicFreestreamFvPatchVectorField::updateCoeffs()"
        )   << "    MachInf < 1.0, free stream must be supersonic"
            << "\n    on patch " << this->patch().name()
            << " of field " << this->dimensionedInternalField().name()
            << " in file " << this->dimensionedInternalField().objectPath()
            << exit(FatalError);
    }

    vectorgpuField& Up = refValue();
    valueFraction() = 1;

    // get the near patch internal cell values
    const vectorgpuField U(patchInternalField());


    // Find the component of U normal to the free-stream flow and in the
    // plane of the free-stream flow and the patch normal

    // Direction of the free-stream flow
    vector UInfHat = UInf_/mag(UInf_);

    // Normal to the plane defined by the free-stream and the patch normal
    tmp<vectorgpuField> nnInfHat = UInfHat ^ patch().nf();

    // Normal to the free-stream in the plane defined by the free-stream
    // and the patch normal
    const vectorgpuField nHatInf(nnInfHat ^ UInfHat);

    // Component of U normal to the free-stream in the plane defined by the
    // free-stream and the patch normal
    const vectorgpuField Un(nHatInf*(nHatInf & U));

    // The tangential component is
    const vectorgpuField Ut(U - Un);

    // Calculate the Prandtl-Meyer function of the free-stream
    scalar nuMachInf =
        sqrt((gamma_ + 1)/(gamma_ - 1))
       *atan(sqrt((gamma_ - 1)/(gamma_ + 1)*(sqr(MachInf) - 1)))
      - atan(sqr(MachInf) - 1);


    // Set the patch boundary condition based on the Mach number and direction
    // of the flow dictated by the boundary/free-stream pressure difference

    forAll(Up, facei)
    {
        if (pp.get(facei) >= pInf_) // If outflow
        {
            // Assume supersonic outflow and calculate the boundary velocity
            // according to ???

            // scalar fpp =
            //     sqrt(sqr(MachInf) - 1)
            //    /(gamma_*sqr(MachInf))*mag(Ut[facei])*log(pp[facei]/pInf_);

            scalar fpp =
                sqrt(sqr(MachInf) - 1)
               /(gamma_*sqr(MachInf))*mag(Ut.get(facei))*log(pp.get(facei)/pInf_);

            // Up[facei] = Ut[facei] + fpp*nHatInf[facei];
            Up.set(facei, Ut.get(facei) + fpp*nHatInf.get(facei));

            // Calculate the Mach number of the boundary velocity
            // scalar Mach = mag(Up[facei])/sqrt(gamma_/ppsi[facei]);
            scalar Mach = mag(Up.get(facei))/sqrt(gamma_/ppsi.get(facei));

            if (Mach <= 1) // If subsonic
            {
                // Zero-gradient subsonic outflow

                // Up[facei] = U[facei];
                // valueFraction()[facei] = 0;

                Up.set(facei, U.get(facei));
                valueFraction().set(facei, 0);
            }
        }
        else // if inflow
        {
            // Calculate the Mach number of the boundary velocity
            // from the boundary pressure assuming constant total pressure
            // expansion from the free-stream
            scalar Mach =
                sqrt
                (
                    (2/(gamma_ - 1))*(1 + ((gamma_ - 1)/2)*sqr(MachInf))
                   *pow(pp.get(facei)/pInf_, (1 - gamma_)/gamma_)
                  - 2/(gamma_ - 1)
                );

            if (Mach > 1) // If supersonic
            {
                // Supersonic inflow is assumed to occur according to the
                // Prandtl-Meyer expansion process

                scalar nuMachf =
                    sqrt((gamma_ + 1)/(gamma_ - 1))
                   *atan(sqrt((gamma_ - 1)/(gamma_ + 1)*(sqr(Mach) - 1)))
                  - atan(sqr(Mach) - 1);

                // scalar fpp = (nuMachInf - nuMachf)*mag(Ut[facei]);
                scalar fpp = (nuMachInf - nuMachf)*mag(Ut.get(facei));

                // Up[facei] = Ut[facei] + fpp*nHatInf[facei];
                Up.set(facei, Ut.get(facei) + fpp*nHatInf.get(facei));
            }
            else // If subsonic
            {
                FatalErrorIn
                (
                    "supersonicFreestreamFvPatchVectorField::updateCoeffs()"
                )   << "unphysical subsonic inflow has been generated"
                    << "\n    on patch " << this->patch().name()
                    << " of field " << this->dimensionedInternalField().name()
                    << " in file "
                    << this->dimensionedInternalField().objectPath()
                    << exit(FatalError);
            }
        }
    }

    mixedFvPatchVectorField::updateCoeffs();
}


void Foam::supersonicFreestreamFvPatchVectorField::write(Ostream& os) const
{
    fvPatchVectorField::write(os);
    writeEntryIfDifferent<word>(os, "T", "T", TName_);
    writeEntryIfDifferent<word>(os, "p", "p", pName_);
    writeEntryIfDifferent<word>(os, "psi", "thermo:psi", psiName_);
    os.writeKeyword("UInf") << UInf_ << token::END_STATEMENT << nl;
    os.writeKeyword("pInf") << pInf_ << token::END_STATEMENT << nl;
    os.writeKeyword("TInf") << TInf_ << token::END_STATEMENT << nl;
    os.writeKeyword("gamma") << gamma_ << token::END_STATEMENT << nl;
    writeEntry("value", os);
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
    makePatchTypeField
    (
        fvPatchVectorField,
        supersonicFreestreamFvPatchVectorField
    );
}

// ************************************************************************* //

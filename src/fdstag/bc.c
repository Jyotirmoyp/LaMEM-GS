//---------------------------------------------------------------------------
//........................... BOUNDARY CONDITIONS ...........................
//---------------------------------------------------------------------------
#include "LaMEM.h"
#include "fdstag.h"
#include "solVar.h"
#include "scaling.h"
#include "tssolve.h"
#include "bc.h"
#include "Utils.h"
#include "tools.h"
//---------------------------------------------------------------------------
// * replace BC input specification consistently in the entire code
// * open box & Winkler (with tangential viscous friction)
// * tangential velocities
// * extend two-point constraint specification & (possibly) get rid bc-vectors
// * create bc-object only at fine level, coarse levels should have simple access
//---------------------------------------------------------------------------
#undef __FUNCT__
#define __FUNCT__ "BCClear"
PetscErrorCode BCClear(BCCtx *bc)
{
	PetscErrorCode ierr;
	PetscFunctionBegin;

	// clear object
	ierr = PetscMemzero(bc, sizeof(BCCtx)); CHKERRQ(ierr);

	PetscFunctionReturn(0);
}
//---------------------------------------------------------------------------
#undef __FUNCT__
#define __FUNCT__ "BCCreate"
PetscErrorCode BCCreate(BCCtx *bc, FDSTAG *fs, TSSol *ts, Scaling *scal)
{
	PetscErrorCode ierr;
	PetscFunctionBegin;

	// create boundary conditions vectors (velocity, pressure, temperature)
	ierr = DMCreateLocalVector(fs->DA_X,   &bc->bcvx);  CHKERRQ(ierr);
	ierr = DMCreateLocalVector(fs->DA_Y,   &bc->bcvy);  CHKERRQ(ierr);
	ierr = DMCreateLocalVector(fs->DA_Z,   &bc->bcvz);  CHKERRQ(ierr);
	ierr = DMCreateLocalVector(fs->DA_CEN, &bc->bcp);   CHKERRQ(ierr);
	ierr = DMCreateLocalVector(fs->DA_CEN, &bc->bcT);   CHKERRQ(ierr);

	// SPC velocity-pressure
	ierr = makeIntArray (&bc->SPCList, NULL, fs->dof.ln);   CHKERRQ(ierr);
	ierr = makeScalArray(&bc->SPCVals, NULL, fs->dof.ln);   CHKERRQ(ierr);

	// SPC (temperature)
	ierr = makeIntArray (&bc->tSPCList, NULL, fs->dof.lnp); CHKERRQ(ierr);
	ierr = makeScalArray(&bc->tSPCVals, NULL, fs->dof.lnp); CHKERRQ(ierr);

	bc->ExxAct = PETSC_FALSE;
	bc->EyyAct = PETSC_FALSE;
	bc->pbAct  = PETSC_FALSE;
	bc->pbApp  = PETSC_FALSE;

	bc->fs   = fs;
	bc->ts   = ts;
	bc->scal = scal;

	PetscFunctionReturn(0);
}
//---------------------------------------------------------------------------
#undef __FUNCT__
#define __FUNCT__ "BCDestroy"
PetscErrorCode BCDestroy(BCCtx *bc)
{
	PetscErrorCode ierr;
	PetscFunctionBegin;

	// destroy boundary conditions vectors (velocity, pressure, temperature)
	ierr = VecDestroy(&bc->bcvx); CHKERRQ(ierr);
	ierr = VecDestroy(&bc->bcvy); CHKERRQ(ierr);
	ierr = VecDestroy(&bc->bcvz); CHKERRQ(ierr);
	ierr = VecDestroy(&bc->bcp);  CHKERRQ(ierr);
	ierr = VecDestroy(&bc->bcT);  CHKERRQ(ierr);

	// SPC velocity-pressure
	ierr = PetscFree(bc->SPCList);  CHKERRQ(ierr);
	ierr = PetscFree(bc->SPCVals);  CHKERRQ(ierr);

	// SPC temperature
	ierr = PetscFree(bc->tSPCList); CHKERRQ(ierr);
	ierr = PetscFree(bc->tSPCVals); CHKERRQ(ierr);

	// two-point constraints
//	ierr = PetscFree(bc->TPCList);      CHKERRQ(ierr);
//	ierr = PetscFree(bc->TPCPrimeDOF);  CHKERRQ(ierr);
//	ierr = PetscFree(bc->TPCVals);      CHKERRQ(ierr);
//	ierr = PetscFree(bc->TPCLinComPar); CHKERRQ(ierr);

	PetscFunctionReturn(0);
}
//---------------------------------------------------------------------------
#undef __FUNCT__
#define __FUNCT__ "BCSetParam"
PetscErrorCode BCSetParam(BCCtx *bc, UserCtx *user)
{
	PetscFunctionBegin;

	bc->Tbot  = user->Temp_bottom;
	bc->Ttop  = user->Temp_top;

	PetscFunctionReturn(0);
}
//---------------------------------------------------------------------------
#undef __FUNCT__
#define __FUNCT__ "BCReadFromOptions"
PetscErrorCode BCReadFromOptions(BCCtx *bc)
{
	// set parameters from PETSc options

	Scaling *scal;

	PetscErrorCode ierr;
	PetscFunctionBegin;

	scal = bc->scal;

	// x-direction background strain rate

	ierr = GetIntDataItemCheck("-ExxNumPeriods", "Number of Exx background strain rate periods",
		_NOT_FOUND_EXIT_, 1, &bc->ExxNumPeriods, 1, _max_periods_); CHKERRQ(ierr);

	if(bc->ExxNumPeriods)
	{
		bc->ExxAct = PETSC_TRUE;

		ierr = GetScalDataItemCheckScale("-ExxTimeDelims", "Exx background strain rate time delimiters",
			_NOT_FOUND_ERROR_, bc->ExxNumPeriods-1, bc->ExxTimeDelims, 0.0, 0.0, scal->time); CHKERRQ(ierr);

		ierr = GetScalDataItemCheckScale("-ExxStrainRates", "Exx background strain rates",
			_NOT_FOUND_ERROR_, bc->ExxNumPeriods, bc->ExxStrainRates, 0.0, 0.0, scal->strain_rate); CHKERRQ(ierr);
	}

	// y-direction background strain rate

	ierr = GetIntDataItemCheck("-EyyNumPeriods", "Number of Eyy background strain rate periods",
		_NOT_FOUND_EXIT_, 1, &bc->EyyNumPeriods, 1, _max_periods_); CHKERRQ(ierr);

	if(bc->EyyNumPeriods)
	{
		bc->EyyAct = PETSC_TRUE;

		ierr = GetScalDataItemCheckScale("-EyyTimeDelims", "Eyy background strain rate time delimiters",
			_NOT_FOUND_ERROR_, bc->EyyNumPeriods-1, bc->EyyTimeDelims, 0.0, 0.0, scal->time); CHKERRQ(ierr);

		ierr = GetScalDataItemCheckScale("-EyyStrainRates", "Eyy background strain rates",
			_NOT_FOUND_ERROR_, bc->EyyNumPeriods, bc->EyyStrainRates, 0.0, 0.0, scal->strain_rate); CHKERRQ(ierr);
	}

	PetscFunctionReturn(0);
}
//---------------------------------------------------------------------------
#undef __FUNCT__
#define __FUNCT__ "BCGetBGStrainRates"
PetscErrorCode BCGetBGStrainRates(BCCtx *bc, PetscScalar *Exx_, PetscScalar *Eyy_, PetscScalar *Ezz_)
{
	// get current background strain rates

	PetscInt    jj;
	PetscScalar time, Exx, Eyy, Ezz;

	// initialize
	time = bc->ts->time;
	Exx  = 0.0;
	Eyy  = 0.0;
	Ezz  = 0.0;

	// x-direction background strain rate
	if(bc->ExxAct == PETSC_TRUE)
	{
		for(jj = 0; jj < bc->ExxNumPeriods-1; jj++)
		{
			if(time < bc->ExxTimeDelims[jj]) break;
		}

		Exx = bc->ExxStrainRates[jj];
	}

	// y-direction background strain rate
	if(bc->EyyAct == PETSC_TRUE)
	{
		for(jj = 0; jj < bc->EyyNumPeriods-1; jj++)
		{
			if(time < bc->EyyTimeDelims[jj]) break;
		}

		Eyy = bc->EyyStrainRates[jj];
	}

	// z-direction background strain rate
	Ezz = -(Exx+Eyy);

	// store result
	if(Exx_) (*Exx_) = Exx;
	if(Eyy_) (*Eyy_) = Eyy;
	if(Ezz_) (*Ezz_) = Ezz;

	PetscFunctionReturn(0);
}
//---------------------------------------------------------------------------
#undef __FUNCT__
#define __FUNCT__ "BCApply"
PetscErrorCode BCApply(BCCtx *bc)
{
	FDSTAG      *fs;
	DOFIndex    *dof;
	PetscScalar *SPCVals;
	PetscInt    i, ln, lnv, numSPC, vNumSPC, pNumSPC, *SPCList;

 	PetscErrorCode ierr;
	PetscFunctionBegin;

	// access context
	fs      = bc->fs;
	dof     = &fs->dof;
	ln      = dof->ln;
	lnv     = dof->lnv;
	SPCList = bc->SPCList;
	SPCVals = bc->SPCVals;

	// mark all variables unconstrained
	ierr = VecSet(bc->bcvx, DBL_MAX); CHKERRQ(ierr);
	ierr = VecSet(bc->bcvy, DBL_MAX); CHKERRQ(ierr);
	ierr = VecSet(bc->bcvz, DBL_MAX); CHKERRQ(ierr);
	ierr = VecSet(bc->bcp,  DBL_MAX); CHKERRQ(ierr);
	ierr = VecSet(bc->bcT,  DBL_MAX); CHKERRQ(ierr);

	for(i = 0; i < ln; i++) SPCVals[i] = DBL_MAX;

	// apply boundary constraints
	ierr = BCApplyBound(bc); CHKERRQ(ierr);

	// compute pushing parameters
	ierr = BCCompPush(bc); CHKERRQ(ierr);

	// apply pushing block constraints
	ierr = BCApplyPush(bc); CHKERRQ(ierr);

	// exchange ghost point constraints
	// AVOID THIS BY SETTING CONSTRAINTS REDUNDANTLY
	// IN MULTIGRID ONLY REPEAT BC COARSENING WHEN THINGS CHANGE
	LOCAL_TO_LOCAL(fs->DA_X,   bc->bcvx)
	LOCAL_TO_LOCAL(fs->DA_Y,   bc->bcvy)
	LOCAL_TO_LOCAL(fs->DA_Z,   bc->bcvz)
	LOCAL_TO_LOCAL(fs->DA_CEN, bc->bcp)
	LOCAL_TO_LOCAL(fs->DA_CEN, bc->bcT)

	// form constraint lists
	numSPC = 0;

	// velocity
	for(i = 0; i < lnv; i++)
	{
		if(SPCVals[i] != DBL_MAX)
		{
			SPCList[numSPC] = i;
			SPCVals[numSPC] = SPCVals[i];
			numSPC++;
		}
	}
	vNumSPC = numSPC;

	// pressure
	for(i = lnv; i < ln; i++)
	{
		if(SPCVals[i] != DBL_MAX)
		{
			SPCList[numSPC] = i;
			SPCVals[numSPC] = SPCVals[i];
			numSPC++;
		}
	}
	pNumSPC = numSPC - vNumSPC;

	// set index (shift) type
	bc->stype = _GLOBAL_TO_LOCAL_;

	// store constraint lists
	bc->numSPC   = numSPC;
	bc->vNumSPC  = vNumSPC;
	bc->vSPCList = SPCList;
	bc->vSPCVals = SPCVals;
	bc->pNumSPC  = pNumSPC;
	bc->pSPCList = SPCList + vNumSPC;
	bc->pSPCVals = SPCVals + vNumSPC;

	// WARNING! currently primary temperature constraints are not implemented
	bc->tNumSPC = 0;

	PetscFunctionReturn(0);
}
//---------------------------------------------------------------------------
#undef __FUNCT__
#define __FUNCT__ "BCShiftIndices"
PetscErrorCode BCShiftIndices(BCCtx *bc, ShiftType stype)
{
	FDSTAG   *fs;
	DOFIndex *dof;
	PetscInt i, vShift, pShift;

	PetscInt vNumSPC, pNumSPC, *vSPCList, *pSPCList;

	// error checking
	if(stype == bc->stype)
	{
		SETERRQ(PETSC_COMM_SELF, PETSC_ERR_USER,"Cannot call same type of index shifting twice in a row");
	}

	// access context
	fs       = bc->fs;
	dof      = &fs->dof;
	vNumSPC  = bc->vNumSPC;
	vSPCList = bc->vSPCList;
	pNumSPC  = bc->pNumSPC;
	pSPCList = bc->pSPCList;

	// get local-to-global index shifts
	if(dof->idxmod == IDXCOUPLED)   { vShift = dof->st;  pShift = dof->st;             }
	if(dof->idxmod == IDXUNCOUPLED) { vShift = dof->stv; pShift = dof->stp - dof->lnv; }

	// shift constraint indices
	if(stype == _LOCAL_TO_GLOBAL_)
	{
		for(i = 0; i < vNumSPC; i++) vSPCList[i] += vShift;
		for(i = 0; i < pNumSPC; i++) pSPCList[i] += pShift;
	}
	else if(stype == _GLOBAL_TO_LOCAL_)
	{
		for(i = 0; i < vNumSPC; i++) vSPCList[i] -= vShift;
		for(i = 0; i < pNumSPC; i++) pSPCList[i] -= pShift;
	}

	// switch shit type
	bc->stype = stype;

	PetscFunctionReturn(0);
}
//---------------------------------------------------------------------------
#undef __FUNCT__
#define __FUNCT__ "BCApplyBound"
PetscErrorCode BCApplyBound(BCCtx *bc)
{
	// initialize boundary conditions vectors

	// *************************************************************
	// WARNING !!!
	//    AD-HOC FREE-SLIP BOX IS CURRENTLY ASSUMED
	//    NORMAL VELOCITIES CAN BE SET VIA BACKGROUND STRAIN RATES
	//    PRESSURE CONSTRAINS ARE CURRENTLY NOT ALLOWED
	//    TEMPERATURE IS PRESCRIBED ON TOP AND BOTTOM BOUNDARIES
	//    ONLY ZERO BOUNDARY HEAT FLUXES ARE IMPLEMENTED
	//    TWO-POINT CONSTRAINTS MUST BE SET ON CROSS-PROCESSOR GHOST POINTS
	// *************************************************************
	FDSTAG      *fs;
	PetscScalar Tbot, Ttop;
	PetscScalar Exx, Eyy, Ezz;
	PetscScalar bx,  by,  bz;
	PetscScalar ex,  ey,  ez;
	PetscScalar vbx, vby, vbz;
	PetscScalar vex, vey, vez;
//	PetscInt    mcx, mcy, mcz;
	PetscInt              mcz;
	PetscInt    mnx, mny, mnz;
	PetscInt    i, j, k, nx, ny, nz, sx, sy, sz, iter;
	PetscScalar ***bcvx,  ***bcvy,  ***bcvz, ***bcT, *SPCVals;

	PetscErrorCode ierr;
	PetscFunctionBegin;

	// access context
	fs = bc->fs;

	// initialize maximal index in all directions
	mnx = fs->dsx.tnods - 1;
	mny = fs->dsy.tnods - 1;
	mnz = fs->dsz.tnods - 1;
//	mcx = fs->dsx.tcels - 1;
//	mcy = fs->dsy.tcels - 1;
	mcz = fs->dsz.tcels - 1;

	// get current coordinates of the mesh boundaries
	ierr = FDSTAGGetGlobalBox(fs, &bx, &by, &bz, &ex, &ey, &ez); CHKERRQ(ierr);

	// get background strain rates
	ierr = BCGetBGStrainRates(bc, &Exx, &Eyy, &Ezz); CHKERRQ(ierr);

	// get boundary velocities
	// coordinate origin is assumed to be fixed
	// velocity is a product of strain rate and coordinate
	vbx = bx*Exx;   vex = ex*Exx;
	vby = by*Eyy;   vey = ey*Eyy;
	vbz = bz*Ezz;   vez = ez*Ezz;

	// get boundary temperatures
	Tbot = bc->Tbot;
	Ttop = bc->Ttop;

	// access constraint vectors
	ierr = DMDAVecGetArray(fs->DA_X,   bc->bcvx, &bcvx); CHKERRQ(ierr);
	ierr = DMDAVecGetArray(fs->DA_Y,   bc->bcvy, &bcvy); CHKERRQ(ierr);
	ierr = DMDAVecGetArray(fs->DA_Z,   bc->bcvz, &bcvz); CHKERRQ(ierr);
	ierr = DMDAVecGetArray(fs->DA_CEN, bc->bcT,  &bcT);  CHKERRQ(ierr);

	// access constraint arrays
	SPCVals = bc->SPCVals;

	iter = 0;

	//------------------
	// X points SPC only
	//------------------
	GET_NODE_RANGE(nx, sx, fs->dsx)
	GET_CELL_RANGE(ny, sy, fs->dsy)
	GET_CELL_RANGE(nz, sz, fs->dsz)

	START_STD_LOOP
	{
		if(i == 0)   { bcvx[k][j][i] = vbx; SPCVals[iter] = vbx; }
		if(i == mnx) { bcvx[k][j][i] = vex; SPCVals[iter] = vex; }
		iter++;
	}
	END_STD_LOOP

	//------------------
	// Y points SPC only
	//------------------
	GET_CELL_RANGE(nx, sx, fs->dsx)
	GET_NODE_RANGE(ny, sy, fs->dsy)
	GET_CELL_RANGE(nz, sz, fs->dsz)

	START_STD_LOOP
	{
		if(j == 0)   { bcvy[k][j][i] = vby; SPCVals[iter] = vby; }
		if(j == mny) { bcvy[k][j][i] = vey; SPCVals[iter] = vey; }
		iter++;
	}
	END_STD_LOOP

	//------------------
	// Z points SPC only
	//------------------
	GET_CELL_RANGE(nx, sx, fs->dsx)
	GET_CELL_RANGE(ny, sy, fs->dsy)
	GET_NODE_RANGE(nz, sz, fs->dsz)

	START_STD_LOOP
	{
		if(k == 0)   { bcvz[k][j][i] = vbz;	SPCVals[iter] = vbz; }
		if(k == mnz) { bcvz[k][j][i] = vez; SPCVals[iter] = vez; }
		iter++;
	}
	END_STD_LOOP

	//-----------------------------------------------------
	// T points (TPC only, hence looping over ghost points)
	//-----------------------------------------------------

	GET_CELL_RANGE_GHOST_INT(nx, sx, fs->dsx)
	GET_CELL_RANGE_GHOST_INT(ny, sy, fs->dsy)
	GET_CELL_RANGE_GHOST_INT(nz, sz, fs->dsz)

	START_STD_LOOP
	{
		if(k == 0)   { bcT[k-1][j][i] = Tbot; }
		if(k == mcz) { bcT[k+1][j][i] = Ttop; }
	}
	END_STD_LOOP

	// restore access
	ierr = DMDAVecRestoreArray(fs->DA_X,   bc->bcvx, &bcvx); CHKERRQ(ierr);
	ierr = DMDAVecRestoreArray(fs->DA_Y,   bc->bcvy, &bcvy); CHKERRQ(ierr);
	ierr = DMDAVecRestoreArray(fs->DA_Z,   bc->bcvz, &bcvz); CHKERRQ(ierr);
	ierr = DMDAVecRestoreArray(fs->DA_CEN, bc->bcT,  &bcT);  CHKERRQ(ierr);

	PetscFunctionReturn(0);
}
//---------------------------------------------------------------------------
#undef __FUNCT__
#define __FUNCT__ "BCSetPush"
PetscErrorCode BCSetPush(BCCtx *bc, UserCtx *user)
{
	PetscFunctionBegin;

	if(user->AddPushing)
	{
		bc->pbAct = PETSC_TRUE;
		bc->pb    = &user->Pushing;
	}

	PetscFunctionReturn(0);
}
//---------------------------------------------------------------------------
#undef __FUNCT__
#define __FUNCT__ "BCCompPush"
PetscErrorCode BCCompPush(BCCtx *bc)
{
	// MUST be called at the beginning of time step before setting boundary conditions
	// compute pushing boundary conditions actual parameters

	PushParams    *pb;
	TSSol         *ts;
	Scaling       *scal;
	PetscInt      i, ichange;
	PetscScalar   Vx, Vy, theta;

	PetscFunctionBegin;

	// check if pushing option is activated
	if(bc->pbAct != PETSC_TRUE) PetscFunctionReturn(0);

	// access contexts
	pb   = bc->pb;
	ts   = bc->ts;
	scal = bc->scal;

	// set boundary conditions flag
	bc->pbApp = PETSC_FALSE;

	// add pushing boundary conditions ONLY within the specified time interval - for that introduce a new flag
	if(ts->time >= pb->time[0]
	&& ts->time <= pb->time[pb->num_changes])
	{
		// check which pushing stage
		ichange = 0;

		for(i = 0; i < pb->num_changes; i++)
		{
			if(ts->time >= pb->time[i] && ts->time <= pb->time[i+1])
			{
				ichange = i;
			}
		}

		pb->ind_change = ichange;

		// initialize parameters for the time step
		Vx    = 0.0;
		Vy    = 0.0;
		theta = pb->theta;

		if(pb->dir[ichange] == 0)
		{
			Vx = cos(theta)*pb->V_push[ichange];
			Vy = sin(theta)*pb->V_push[ichange];
		}

		if(pb->dir[ichange] == 1) Vx = pb->V_push[ichange];
		if(pb->dir[ichange] == 2) Vy = pb->V_push[ichange];

		// set boundary conditions parameters
		bc->pbApp  = PETSC_TRUE;
		bc->Vx     = Vx;
		bc->Vy     = Vy;
		bc->theta  = theta;

		PetscPrintf(PETSC_COMM_WORLD,"Pushing BC: Time interval=[%g-%g] %s, V_push=%g %s, Omega=%g %s, mobile_block=%lld, direction=%lld, theta=%g deg\n",
			pb->time             [ichange  ]*scal->time,
			pb->time             [ichange+1]*scal->time,             scal->lbl_time,
			pb->V_push           [ichange  ]*scal->velocity,         scal->lbl_velocity,
			pb->omega            [ichange  ]*scal->angular_velocity, scal->lbl_angular_velocity,
			(LLD)pb->coord_advect[ichange  ],
			(LLD)pb->dir         [ichange  ],
			theta*scal->angle);

		PetscPrintf(PETSC_COMM_WORLD,"Pushing BC: Block center coordinates [x, y, z]=[%g, %g, %g] %s\n",
			pb->x_center_block*scal->length,
			pb->y_center_block*scal->length,
			pb->z_center_block*scal->length, scal->lbl_length);
	}
	PetscFunctionReturn(0);
}
//---------------------------------------------------------------------------
#undef __FUNCT__
#define __FUNCT__ "BCApplyPush"
PetscErrorCode BCApplyPush(BCCtx *bc)
{
	// initialize internal (pushing) boundary conditions vectors
	// only x, y velocities are constrained!!
	// constraining vz makes a bad case - will not converge.

	FDSTAG      *fs;
	PushParams  *pb;
	PetscScalar xc, yc, zc;
	PetscScalar	dx, dy, dz;
	PetscScalar px, py, pz;
	PetscScalar rx, ry, rz;
	PetscScalar costh, sinth;
	PetscScalar ***bcvx,  ***bcvy, *SPCVals;
	PetscInt    i, j, k, nx, ny, nz, sx, sy, sz, iter;

	PetscErrorCode ierr;
	PetscFunctionBegin;

	// check whether pushing is applied
	if(bc->pbApp != PETSC_TRUE) PetscFunctionReturn(0);

	// prepare block coordinates, sizes & rotation angle parameters
	fs    = bc->fs;
	pb    = bc->pb;
    xc    = pb->x_center_block;
	yc    = pb->y_center_block;
	zc    = pb->z_center_block;
	dx    = pb->L_block/2.0;
	dy    = pb->W_block/2.0;
	dz    = pb->H_block/2.0;
	costh = cos(bc->theta);
	sinth = sin(bc->theta);

	// access velocity constraint vectors
	ierr = DMDAVecGetArray(fs->DA_X, bc->bcvx, &bcvx); CHKERRQ(ierr);
	ierr = DMDAVecGetArray(fs->DA_Y, bc->bcvy, &bcvy); CHKERRQ(ierr);

	// access constraint arrays
	SPCVals = bc->SPCVals;

	iter = 0;

	//---------
	// X points
	//---------
	GET_NODE_RANGE(nx, sx, fs->dsx)
	GET_CELL_RANGE(ny, sy, fs->dsy)
	GET_CELL_RANGE(nz, sz, fs->dsz)

	START_STD_LOOP
	{
		// get point coordinates in the block-centered system
		px = COORD_NODE(i, sx, fs->dsx) - xc;
		py = COORD_CELL(j, sy, fs->dsy) - yc;
		pz = COORD_CELL(k, sz, fs->dsz) - zc;

		// get point coordinates in the block-aligned system
		// rotation matrix R = [ cos() sin() ; -sin() cos() ]
		rx =  costh*px + sinth*py;
		ry = -sinth*px + costh*py;
		rz =  pz;

		// perform point test
		if(rx >= -dx && rx <= dx
		&& ry >= -dy && ry <= dy
		&& rz >= -dz && rz <= dz)
		{
			bcvx[k][j][i] = bc->Vx;
			SPCVals[iter] = bc->Vx;
		}
		iter++;
	}
	END_STD_LOOP

	//---------
	// Y points
	//---------
	GET_CELL_RANGE(nx, sx, fs->dsx)
	GET_NODE_RANGE(ny, sy, fs->dsy)
	GET_CELL_RANGE(nz, sz, fs->dsz)

	START_STD_LOOP
	{
		// get point coordinates in the block-centered system
		px = COORD_CELL(i, sx, fs->dsx) - xc;
		py = COORD_NODE(j, sy, fs->dsy) - yc;
		pz = COORD_CELL(k, sz, fs->dsz) - zc;

		// get point coordinates in the block-aligned system
		// rotation matrix R = [ cos() sin() ; -sin() cos() ]
		rx =  costh*px + sinth*py;
		ry = -sinth*px + costh*py;
		rz =  pz;

		// perform point test
		if(rx >= -dx && rx <= dx
		&& ry >= -dy && ry <= dy
		&& rz >= -dz && rz <= dz)
		{
			bcvy[k][j][i] = bc->Vy;
			SPCVals[iter] = bc->Vy;
		}
		iter++;
	}
	END_STD_LOOP

	// restore access
	ierr = DMDAVecRestoreArray(fs->DA_X, bc->bcvx, &bcvx); CHKERRQ(ierr);
	ierr = DMDAVecRestoreArray(fs->DA_Y, bc->bcvy, &bcvy); CHKERRQ(ierr);

	PetscFunctionReturn(0);
}
//---------------------------------------------------------------------------
#undef __FUNCT__
#define __FUNCT__ "BCAdvectPush"
PetscErrorCode BCAdvectPush(BCCtx *bc)
{
	TSSol        *ts;
	PushParams   *pb;
	PetscInt     advc, ichange;
	PetscScalar  xc, yc, zc, Vx, Vy, dx, dy, dt, omega, theta, dtheta;

	// check if pushing option is activated
	if(bc->pbAct != PETSC_TRUE) PetscFunctionReturn(0);

	// access context
	pb = bc->pb;
	ts = bc->ts;

	// check time interval
	if(ts->time >= pb->time[0]
	&& ts->time <= pb->time[pb->num_changes])
	{
		// initialize variables
		ichange = pb->ind_change;
		advc    = pb->coord_advect[ichange];

		Vx = 0.0;
		Vy = 0.0;
		dy = 0.0;
		dx = 0.0;
		dt = ts->dt;

		xc = pb->x_center_block;
		yc = pb->y_center_block;
		zc = pb->z_center_block;

		// block is rotated
		if(pb->dir[ichange] == 0)
		{
			omega = pb->omega[ichange];
			theta = pb->theta;
			Vx    = cos(theta)*pb->V_push[ichange];
			Vy    = sin(theta)*pb->V_push[ichange];

			// rotation
			dtheta = omega*dt;
			pb->theta = theta + dtheta;
		}

		// block moves in X-direction
		if(pb->dir[ichange] == 1) Vx = pb->V_push[ichange];

		// block moves in Y-direction
		if(pb->dir[ichange] == 2) Vy = pb->V_push[ichange];

		// get displacements
		dx = Vx*dt;
		dy = Vy*dt;

		if(advc == 0)
		{	// stationary block
			pb->x_center_block = xc;
			pb->y_center_block = yc;
			pb->z_center_block = zc;
		}
		else
		{	// moving block
			pb->x_center_block = xc + dx;
			pb->y_center_block = yc + dy;
			pb->z_center_block = zc;
		}
	}

	PetscFunctionReturn(0);
}
//---------------------------------------------------------------------------
#undef __FUNCT__
#define __FUNCT__ "BCStretchGrid"
PetscErrorCode BCStretchGrid(BCCtx *bc)
{
	// apply background strain-rate "DWINDLAR" BC (Bob Shaw "Ship of Strangers")

	// Stretch grid with constant stretch factor about coordinate origin.
	// The origin point remains fixed, and the displacements of all points are
	// proportional to the distance from the origin (i.e. coordinate).
	// The origin (zero) point must remain within domain (checked at input).
	// Stretch factor is positive at extension, i.e.:
	// eps = (L_new-L_old)/L_old
	// L_new = L_old + eps*L_old
	// x_new = x_old + eps*x_old

	TSSol       *ts;
	FDSTAG      *fs;
	PetscScalar Exx, Eyy, Ezz;

	PetscErrorCode ierr;
	PetscFunctionBegin;

	// access context
	fs = bc->fs;
	ts = bc->ts;

	// get background strain rates
	ierr = BCGetBGStrainRates(bc, &Exx, &Eyy, &Ezz); CHKERRQ(ierr);

	// stretch grid
	if(Exx) { ierr = Discret1DStretch(&fs->dsx, &fs->msx, Exx*ts->dt); CHKERRQ(ierr); }
	if(Eyy) { ierr = Discret1DStretch(&fs->dsy, &fs->msy, Eyy*ts->dt); CHKERRQ(ierr); }
	if(Ezz) { ierr = Discret1DStretch(&fs->dsz, &fs->msz, Ezz*ts->dt); CHKERRQ(ierr); }

	PetscFunctionReturn(0);
}
//---------------------------------------------------------------------------

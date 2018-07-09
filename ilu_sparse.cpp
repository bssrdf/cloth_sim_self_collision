﻿//
//  ilu_sparse.cpp
//  internal_cloth_sparse
//
//  Created by Nobuyuki Umetani on 11/18/13.
//  Copyright (c) 2013 Nobuyuki Umetani. All rights reserved.
//

#include <assert.h>
#include <math.h>
#include "ilu_sparse.h"


static void CalcMatPr(double* out, const double* d, double* tmp,
                      const int ni, const int nj )
{
	int i,j,k;
	for(i=0;i<ni;i++){
		for(j=0;j<nj;j++){
			tmp[i*nj+j] = 0.0;
			for(k=0;k<ni;k++){
				tmp[i*nj+j] += d[i*ni+k]*out[k*nj+j];
			}
		}
	}
	for(i=0;i<ni*nj;i++){
		out[i] = tmp[i];
	}
}

static void CalcSubMatPr(double* out, const double* a, const double* b,
                         const int ni, const int nk, const int nj )
{
	int i,j,k;
	for(i=0;i<ni;i++){
		for(j=0;j<nj;j++){
			for(k=0;k<nk;k++){
				out[i*nj+j] -= a[i*nk+k]*b[k*nj+j];
			}
		}
	}
}


static void CalcInvMat(double* a, const int n, int& info )
{
	double tmp1;
  
	info = 0;
	int i,j,k;
	for(i=0;i<n;i++){
		if( fabs(a[i*n+i]) < 1.0e-30 ){
			info = 1;
			return;
		}
		if( a[i*n+i] < 0.0 ){
			info--;
		}
		tmp1 = 1.0 / a[i*n+i];
		a[i*n+i] = 1.0;
		for(k=0;k<n;k++){
			a[i*n+k] *= tmp1;
		}
		for(j=0;j<n;j++){
			if( j!=i ){
				tmp1 = a[j*n+i];
				a[j*n+i] = 0.0;
				for(k=0;k<n;k++){
					a[j*n+k] -= tmp1*a[i*n+k];
				}
			}
		}
	}
}

// t is a tmporary buffer size of 9
static inline void CalcInvMat3(double a[], double t[] )
{
	const double det =
  + a[0]*a[4]*a[8] + a[3]*a[7]*a[2] + a[6]*a[1]*a[5]
  - a[0]*a[7]*a[5] - a[6]*a[4]*a[2] - a[3]*a[1]*a[8];
	const double inv_det = 1.0/det;
  
  for(int i=0;i<9;i++){ t[i] = a[i]; }
  
	a[0] = inv_det*(t[4]*t[8]-t[5]*t[7]);
	a[1] = inv_det*(t[2]*t[7]-t[1]*t[8]);
	a[2] = inv_det*(t[1]*t[5]-t[2]*t[4]);
  
	a[3] = inv_det*(t[5]*t[6]-t[3]*t[8]);
	a[4] = inv_det*(t[0]*t[8]-t[2]*t[6]);
	a[5] = inv_det*(t[2]*t[3]-t[0]*t[5]);
  
	a[6] = inv_det*(t[3]*t[7]-t[4]*t[6]);
	a[7] = inv_det*(t[1]*t[6]-t[0]*t[7]);
	a[8] = inv_det*(t[0]*t[4]-t[1]*t[3]);
}

/* --------------------------------------------------------------------- */


CPreconditionerILU::CPreconditionerILU()
{
  
}


CPreconditionerILU::~CPreconditionerILU()
{
  if( m_diaInd != 0 ){ delete[] m_diaInd; m_diaInd = 0; }
}


void CPreconditionerILU::Initialize_ILU0
(const CMatrixSquareSparse& m)
{
  this->mat = m;
  const int nblk = m.m_nblk;
  
  if( m_diaInd != 0 ){ delete[] m_diaInd; m_diaInd = 0; }
  m_diaInd = new int [nblk];  
  for(int iblk=0;iblk<nblk;iblk++){
    m_diaInd[iblk] = mat.m_colInd[iblk+1];
    for(int icrs=mat.m_colInd[iblk];icrs<mat.m_colInd[iblk+1];icrs++){
      assert( icrs < mat.m_ncrs );
      const int jblk0 = mat.m_rowPtr[icrs];
      assert( jblk0 < nblk );
      if( jblk0 > iblk ){
        m_diaInd[iblk] = icrs;
        break;
      }
    }
  }
}

void CPreconditionerILU::ForwardSubstitution( std::vector<double>& vec ) const
{
  const int len = mat.m_len;
  const int nblk = mat.m_nblk;
  
	if( len == 1 ){
		const int* colind = mat.m_colInd;
		const int* rowptr = mat.m_rowPtr;
		const double* vcrs = mat.m_valCrs;
		const double* vdia = mat.m_valDia;
		////////////////    
		for(int iblk=0;iblk<nblk;iblk++){	// ëOêiè¡ãé
			double lvec_i = vec[iblk];
			for(int ijcrs=colind[iblk];ijcrs<m_diaInd[iblk];ijcrs++){
				assert( ijcrs<mat.m_ncrs );
				const int jblk0 = rowptr[ijcrs];
				assert( jblk0<iblk );
				lvec_i -= vcrs[ijcrs]*vec[jblk0];
			}
			vec[iblk] = vdia[iblk]*lvec_i;
		}
	}
	else if( len == 2 ){
		const int* colind = mat.m_colInd;
		const int* rowptr = mat.m_rowPtr;
		const double* vcrs = mat.m_valCrs;
		const double* vdia = mat.m_valDia;
		////////////////
		double pTmpVec[2];
		for(int iblk=0;iblk<nblk;iblk++){
			pTmpVec[0] = vec[iblk*2+0];
			pTmpVec[1] = vec[iblk*2+1];
			const int icrs0 = colind[iblk];
			const int icrs1 = m_diaInd[iblk];
			for(int ijcrs=icrs0;ijcrs<icrs1;ijcrs++){
				assert( ijcrs<mat.m_ncrs );
				const int jblk0 = rowptr[ijcrs];
				assert( jblk0<iblk );
				const double* vij = &vcrs[ijcrs*4];
        const double valj0 = vec[jblk0*2+0];
				const double valj1 = vec[jblk0*2+1];
				pTmpVec[0] -= vij[0]*valj0+vij[1]*valj1;
				pTmpVec[1] -= vij[2]*valj0+vij[3]*valj1;
			}
			const double* vii = &vdia[iblk*4];
			vec[iblk*2+0] = vii[0]*pTmpVec[0]+vii[1]*pTmpVec[1];
			vec[iblk*2+1] = vii[2]*pTmpVec[0]+vii[3]*pTmpVec[1];
		}
	}
	else if( len == 3 ){
		const int* colind = mat.m_colInd;
		const int* rowptr = mat.m_rowPtr;
		const double* vcrs = mat.m_valCrs;
		const double* vdia = mat.m_valDia;
		////////////////
		double pTmpVec[3];
		for(int iblk=0;iblk<nblk;iblk++){
			pTmpVec[0] = vec[iblk*3+0];
			pTmpVec[1] = vec[iblk*3+1];
			pTmpVec[2] = vec[iblk*3+2];
			const int icrs0 = colind[iblk];
			const int icrs1 = m_diaInd[iblk];
			for(int ijcrs=icrs0;ijcrs<icrs1;ijcrs++){
				assert( ijcrs<mat.m_ncrs );
				const int jblk0 = rowptr[ijcrs];
				assert( jblk0<iblk );
				const double* vij = &vcrs[ijcrs*9];
				const double valj0 = vec[jblk0*3+0];
				const double valj1 = vec[jblk0*3+1];
				const double valj2 = vec[jblk0*3+2];
				pTmpVec[0] -= vij[0]*valj0+vij[1]*valj1+vij[2]*valj2;
				pTmpVec[1] -= vij[3]*valj0+vij[4]*valj1+vij[5]*valj2;
				pTmpVec[2] -= vij[6]*valj0+vij[7]*valj1+vij[8]*valj2;
			}
			const double* vii = &vdia[iblk*9];
			vec[iblk*3+0] = vii[0]*pTmpVec[0]+vii[1]*pTmpVec[1]+vii[2]*pTmpVec[2];
			vec[iblk*3+1] = vii[3]*pTmpVec[0]+vii[4]*pTmpVec[1]+vii[5]*pTmpVec[2];
			vec[iblk*3+2] = vii[6]*pTmpVec[0]+vii[7]*pTmpVec[1]+vii[8]*pTmpVec[2];
		}
	}
	else{
		const int blksize = len*len;
		double* pTmpVec = new double [len];
		for(int iblk=0;iblk<nblk;iblk++){
			for(int idof=0;idof<len;idof++){
				pTmpVec[idof] = vec[iblk*len+idof];
			}
			for(int ijcrs=mat.m_colInd[iblk];ijcrs<m_diaInd[iblk];ijcrs++){
				assert( ijcrs<mat.m_ncrs );
				const int jblk0 = mat.m_rowPtr[ijcrs];
				assert( jblk0<iblk );
				const double* vij = &mat.m_valCrs[ijcrs*blksize];
				for(int idof=0;idof<len;idof++){
					for(int jdof=0;jdof<len;jdof++){
						pTmpVec[idof] -= vij[idof*len+jdof]*vec[jblk0*len+jdof];
					}
				}
			}
			const double* vii = &mat.m_valDia[iblk*blksize];
			for(int idof=0;idof<len;idof++){
				double dtmp1 = 0.0;
				for(int jdof=0;jdof<len;jdof++){
					dtmp1 += vii[idof*len+jdof]*pTmpVec[jdof];
				}
				vec[iblk*len+idof] = dtmp1;
			}
		}
		delete[] pTmpVec;
	}
}

void CPreconditionerILU::BackwardSubstitution( std::vector<double>& vec ) const
{
  const int len = mat.m_len;
  const int nblk = mat.m_nblk;

	if( len == 1 ){
		const int* colind = mat.m_colInd;
		const int* rowptr = mat.m_rowPtr;
		const double* vcrs = mat.m_valCrs;
		////////////////
		for(int iblk=nblk-1;iblk>=0;iblk--){	
			assert( (int)iblk < nblk );
			double lvec_i = vec[iblk];
			for(int ijcrs=m_diaInd[iblk];ijcrs<colind[iblk+1];ijcrs++){
				assert( ijcrs<mat.m_ncrs );
				const int jblk0 = rowptr[ijcrs];
				assert( jblk0>(int)iblk && jblk0<nblk );
				lvec_i -=  vcrs[ijcrs]*vec[jblk0];
			}
			vec[iblk] = lvec_i;
		}
	}
	else if( len == 2 ){
		const int* colind = mat.m_colInd;
		const int* rowptr = mat.m_rowPtr;
		const double* vcrs = mat.m_valCrs;
    ////////////////
		double pTmpVec[2];
		for(int iblk=nblk-1;iblk>=0;iblk--){
			assert( (int)iblk < nblk );
			pTmpVec[0] = vec[iblk*2+0];
			pTmpVec[1] = vec[iblk*2+1];
			const int icrs0 = m_diaInd[iblk];
			const int icrs1 = colind[iblk+1];
			for(int ijcrs=icrs0;ijcrs<icrs1;ijcrs++){
				assert( ijcrs<mat.m_ncrs );
				const int jblk0 = rowptr[ijcrs];
				assert( jblk0>(int)iblk && jblk0<nblk );
				const double* vij = &vcrs[ijcrs*4];
				const double valj0 = vec[jblk0*2+0];
				const double valj1 = vec[jblk0*2+1];
				pTmpVec[0] -= vij[0]*valj0+vij[1]*valj1;
				pTmpVec[1] -= vij[2]*valj0+vij[3]*valj1;
			}
			vec[iblk*2+0] = pTmpVec[0];
			vec[iblk*2+1] = pTmpVec[1];
		}
	}
	else if( len == 3 ){
    const int* colind = mat.m_colInd;
		const int* rowptr = mat.m_rowPtr;
		const double* vcrs = mat.m_valCrs;
    ////////////////
		double pTmpVec[3];
		for(int iblk=nblk-1;iblk>=0;iblk--){
			assert( (int)iblk < nblk );
			pTmpVec[0] = vec[iblk*3+0];
			pTmpVec[1] = vec[iblk*3+1];
			pTmpVec[2] = vec[iblk*3+2];
			const int icrs0 = m_diaInd[iblk];
			const int icrs1 = colind[iblk+1];
			for(int ijcrs=icrs0;ijcrs<icrs1;ijcrs++){
				assert( ijcrs<mat.m_ncrs );
				const int jblk0 = rowptr[ijcrs];
				assert( jblk0>(int)iblk && jblk0<nblk );
				const double* vij = &vcrs[ijcrs*9];
				const double valj0 = vec[jblk0*3+0];
				const double valj1 = vec[jblk0*3+1];
				const double valj2 = vec[jblk0*3+2];
				pTmpVec[0] -= vij[0]*valj0+vij[1]*valj1+vij[2]*valj2;
				pTmpVec[1] -= vij[3]*valj0+vij[4]*valj1+vij[5]*valj2;
				pTmpVec[2] -= vij[6]*valj0+vij[7]*valj1+vij[8]*valj2;
			}
			vec[iblk*3+0] = pTmpVec[0];
			vec[iblk*3+1] = pTmpVec[1];
			vec[iblk*3+2] = pTmpVec[2];
		}
	}
	else{
		const int blksize = len*len;
		double* pTmpVec = new double [len];	// çÏã∆ópÇÃè¨Ç≥Ç»îzóÒ
		for(int iblk=nblk-1;iblk>=0;iblk--){
			assert( (int)iblk < nblk );
			for(int idof=0;idof<len;idof++){
				pTmpVec[idof] = vec[iblk*len+idof];
			}
			for(int ijcrs=m_diaInd[iblk];ijcrs<mat.m_colInd[iblk+1];ijcrs++){
				assert( ijcrs<mat.m_ncrs );
				const int jblk0 = mat.m_rowPtr[ijcrs];
				assert( jblk0>(int)iblk && jblk0<nblk );
				const double* vij = &mat.m_valCrs[ijcrs*blksize];
				for(int idof=0;idof<len;idof++){
					for(int jdof=0;jdof<len;jdof++){
						pTmpVec[idof] -= vij[idof*len+jdof]*vec[jblk0*len+jdof];
					}
				}
			}
			for(int idof=0;idof<len;idof++){
				vec[iblk*len+idof] = pTmpVec[idof];
			}
		}
		delete[] pTmpVec;
	}
}

void CPreconditionerILU::SetValueILU(const CMatrixSquareSparse& m)
{
  const int nblk = mat.m_nblk;
	const int len = mat.m_len;
	const int blksize = len*len;
//  for(int i=0;i<mat.m_ncrs*blksize;i++){ mat.m_valCrs[i] = m.m_valCrs[i]; }
  std::vector<int> row2crs(nblk,-1);
  for(int iblk=0;iblk<nblk;iblk++){
    for(int ijcrs=mat.m_colInd[iblk];ijcrs<mat.m_colInd[iblk+1];ijcrs++){
      assert( ijcrs<mat.m_ncrs );
      const int jblk0 = mat.m_rowPtr[ijcrs];
      assert( jblk0 < nblk );
      row2crs[jblk0] = ijcrs;
    }
    for(int ijcrs=m.m_colInd[iblk];ijcrs<m.m_colInd[iblk+1];ijcrs++){
      assert( ijcrs<m.m_ncrs );
      const int jblk0 = m.m_rowPtr[ijcrs];
      assert( jblk0<nblk );
      const int ijcrs0 = row2crs[jblk0];
      if( ijcrs0 == -1 ) continue;
      const double* pval_in = &m.m_valCrs[ijcrs*blksize];
      double* pval_out = &mat.m_valCrs[ijcrs0*blksize];
      for(int i=0;i<blksize;i++){ *(pval_out+i) = *(pval_in+i); }
    }
    for(int ijcrs=mat.m_colInd[iblk];ijcrs<mat.m_colInd[iblk+1];ijcrs++){
      assert( ijcrs<mat.m_ncrs );
      const int jblk0 = mat.m_rowPtr[ijcrs];
      assert( jblk0 < nblk );
      row2crs[jblk0] = -1;
    }
  }
  for(int i=0;i<nblk*blksize;i++){ mat.m_valDia[i] = m.m_valDia[i]; }
}

// numerical factorization
void CPreconditionerILU::DoILUDecomp()
{
  const int nmax_sing = 10;
	int icnt_sing = 0;
  
	const int len = mat.m_len;
  const int nblk = mat.m_nblk;
  const int m_ncrs = mat.m_ncrs;
  const int* colind = mat.m_colInd;
  const int* rowptr = mat.m_rowPtr;
  double* vcrs = mat.m_valCrs;
  double* vdia = mat.m_valDia;
  
  std::vector<int> row2crs(nblk,-1);
  
	if( len == 1 ){
		for(int iblk=0;iblk<nblk;iblk++){
      for(int ijcrs=colind[iblk];ijcrs<colind[iblk+1];ijcrs++){ assert( ijcrs<m_ncrs );
				const int jblk0 = rowptr[ijcrs]; assert( jblk0<nblk );
				row2crs[jblk0] = ijcrs;
			}
			// [L] * [D^-1*U] 
			for(int ikcrs=colind[iblk];ikcrs<m_diaInd[iblk];ikcrs++){
				const int kblk = rowptr[ikcrs]; assert( kblk<nblk );
				const double ikvalue = vcrs[ikcrs];
				for(int kjcrs=m_diaInd[kblk];kjcrs<colind[kblk+1];kjcrs++){
					const int jblk0 = rowptr[kjcrs]; assert( jblk0<nblk );
					if( jblk0 != iblk ){
						const int ijcrs0 = row2crs[jblk0];
						if( ijcrs0 == -1 ) continue;
						vcrs[ijcrs0] -= ikvalue*vcrs[kjcrs];
					}
					else{ vdia[iblk] -= ikvalue*vcrs[kjcrs]; }
				}
			}
			double iivalue = vdia[iblk];
			if( fabs(iivalue) > 1.0e-30 ){
				vdia[iblk] = 1.0 / iivalue;
			}
			else{
				std::cout << "frac false" << iblk << std::endl;
				icnt_sing++;
				if( icnt_sing > nmax_sing ){
					return;
				}
			}
      for(int ijcrs=m_diaInd[iblk];ijcrs<colind[iblk+1];ijcrs++){ assert( ijcrs<m_ncrs );
				vcrs[ijcrs] = vcrs[ijcrs] * vdia[iblk];
			}
      for(int ijcrs=colind[iblk];ijcrs<colind[iblk+1];ijcrs++){ assert( ijcrs<m_ncrs );
				const int jblk0 = rowptr[ijcrs]; assert( jblk0<nblk );
				row2crs[jblk0] = -1;
			}
		}	// end iblk
	}
	////////////////////////////////////////////////////////////////
	else if( len == 2 ){
		double TmpBlk[4];
		for(int iblk=0;iblk<nblk;iblk++){
      for(int ijcrs=colind[iblk];ijcrs<colind[iblk+1];ijcrs++){ assert( ijcrs<m_ncrs );
				const int jblk0 = rowptr[ijcrs]; assert( jblk0<nblk );
				row2crs[jblk0] = ijcrs;
			}
			// [L] * [D^-1*U]
			for(int ikcrs=colind[iblk];ikcrs<m_diaInd[iblk];ikcrs++){
				const int kblk = rowptr[ikcrs]; assert( kblk<nblk );
				const double* vik = &vcrs[ikcrs*4];
				for(int kjcrs=m_diaInd[kblk];kjcrs<colind[kblk+1];kjcrs++){
					const int jblk0 = rowptr[kjcrs]; assert( jblk0<nblk );
					double* vkj = &vcrs[kjcrs*4]; assert( vkj != 0 );
					double* vij = 0;
					if( jblk0 != iblk ){
						const int ijcrs0 = row2crs[jblk0];
						if( ijcrs0 == -1 ) continue;	
						vij = &vcrs[ijcrs0*4];
					}
          else{ vij = &vdia[iblk*4]; }
          assert( vij != 0 );
					vij[0] -= vik[0]*vkj[0]+vik[1]*vkj[2];
					vij[1] -= vik[0]*vkj[1]+vik[1]*vkj[3];
					vij[2] -= vik[2]*vkj[0]+vik[3]*vkj[2];
					vij[3] -= vik[2]*vkj[1]+vik[3]*vkj[3];
				}
			}
			{
				double* vii = &vdia[iblk*4];
				const double det = vii[0]*vii[3]-vii[1]*vii[2];
				if( fabs(det) > 1.0e-30 ){
					const double inv_det = 1.0/det;
					double dtmp1 = vii[0];
					vii[0] =  inv_det*vii[3];
					vii[1] = -inv_det*vii[1];
					vii[2] = -inv_det*vii[2];
					vii[3] =  inv_det*dtmp1;
				}
				else{
					std::cout << "frac false" << iblk << std::endl;
					icnt_sing++;
					if( icnt_sing > nmax_sing ){
						return;
					}
				}
			}
			// [U] = [1/D][U]
      for(int ijcrs=m_diaInd[iblk];ijcrs<colind[iblk+1];ijcrs++){ assert( ijcrs<m_ncrs );
				double* pVal_ij = &vcrs[ijcrs*4];
				const double* vii = &vdia[iblk*4];
				for(int i=0;i<4;i++){ TmpBlk[i] = pVal_ij[i]; }
				pVal_ij[0] = vii[0]*TmpBlk[0] + vii[1]*TmpBlk[2];
				pVal_ij[1] = vii[0]*TmpBlk[1] + vii[1]*TmpBlk[3];
				pVal_ij[2] = vii[2]*TmpBlk[0] + vii[3]*TmpBlk[2];
				pVal_ij[3] = vii[2]*TmpBlk[1] + vii[3]*TmpBlk[3];
			}
      for(int ijcrs=colind[iblk];ijcrs<colind[iblk+1];ijcrs++){ assert( ijcrs<m_ncrs );
				const int jblk0 = rowptr[ijcrs]; assert( jblk0<nblk );
				row2crs[jblk0] = -1;
			}
		}	// end iblk
	}
	////////////////////////////////////////////////////////////////
	else if( len == 3 ){	// lenBlk >= 3
		double tmpBlk[9];
		for(int iblk=0;iblk<nblk;iblk++){
      for(int ijcrs=colind[iblk];ijcrs<colind[iblk+1];ijcrs++){ assert( ijcrs<m_ncrs );
				const int jblk0 = rowptr[ijcrs]; assert( jblk0<nblk );
				row2crs[jblk0] = ijcrs;
			}
			// [L] * [D^-1*U]
			for(int ikcrs=colind[iblk];ikcrs<m_diaInd[iblk];ikcrs++){
				const int kblk = rowptr[ikcrs]; assert( kblk<nblk );
				const double* vik = &vcrs[ikcrs*9];
				for(int kjcrs=m_diaInd[kblk];kjcrs<colind[kblk+1];kjcrs++){
					const int jblk0 = rowptr[kjcrs]; assert( jblk0<nblk );
					double* vkj = &vcrs[kjcrs*9]; assert( vkj != 0 );
					double* vij = 0;
					if( jblk0 != iblk ){
						const int ijcrs0 = row2crs[jblk0];
            if( ijcrs0 == -1 ){ continue; }
						vij = &vcrs[ijcrs0*9];
					}
          else{ vij = &vdia[iblk*9]; }
					assert( vij != 0 );
          for(int i=0;i<3;i++){
            vij[i*3+0] -= vik[i*3+0]*vkj[0] + vik[i*3+1]*vkj[3] + vik[i*3+2]*vkj[6];
            vij[i*3+1] -= vik[i*3+0]*vkj[1] + vik[i*3+1]*vkj[4] + vik[i*3+2]*vkj[7];
            vij[i*3+2] -= vik[i*3+0]*vkj[2] + vik[i*3+1]*vkj[5] + vik[i*3+2]*vkj[8];
          }
				}
			}
			{  
				double* vii = &vdia[iblk*9];
				const double det =
        + vii[0]*vii[4]*vii[8] + vii[3]*vii[7]*vii[2] + vii[6]*vii[1]*vii[5]
        - vii[0]*vii[7]*vii[5] - vii[6]*vii[4]*vii[2] - vii[3]*vii[1]*vii[8];
				if( fabs(det) > 1.0e-30 ){
					CalcInvMat3(vii,tmpBlk);
				}
				else{
					std::cout << "frac false 3 " << iblk << std::endl;
					icnt_sing++;
					if( icnt_sing > nmax_sing ){
            std::cout << "ilu frac false exceeds tolerance" << std::endl;
						return;
					}
				}
			}
			// [U] = [1/D][U]
      for(int ijcrs=m_diaInd[iblk];ijcrs<colind[iblk+1];ijcrs++){ assert( ijcrs<m_ncrs );
				double* vij = &vcrs[ijcrs*9];
				const double* vii = &vdia[iblk*9];
        for(int i=0;i<9;i++){ tmpBlk[i] = vij[i]; }
        for(int i=0;i<3;i++){
          vij[i*3+0] = vii[i*3+0]*tmpBlk[0] + vii[i*3+1]*tmpBlk[3] + vii[i*3+2]*tmpBlk[6];
          vij[i*3+1] = vii[i*3+0]*tmpBlk[1] + vii[i*3+1]*tmpBlk[4] + vii[i*3+2]*tmpBlk[7];
          vij[i*3+2] = vii[i*3+0]*tmpBlk[2] + vii[i*3+1]*tmpBlk[5] + vii[i*3+2]*tmpBlk[8];
        }
			}		
      for(int ijcrs=colind[iblk];ijcrs<colind[iblk+1];ijcrs++){ assert( ijcrs<m_ncrs );
				const int jblk0 = rowptr[ijcrs]; assert( jblk0<nblk );
				row2crs[jblk0] = -1;
			}
		}	// end iblk
	}
  ////////////////////////////////////////////////////////////////
	else{	// lenBlk >= 4
    const int blksize = len*len;
		double* pTmpBlk = new double [blksize];
		for(int iblk=0;iblk<nblk;iblk++){
      for(int ijcrs=colind[iblk];ijcrs<colind[iblk+1];ijcrs++){ assert( ijcrs<m_ncrs );
				const int jblk0 = rowptr[ijcrs]; assert( jblk0<nblk );
				row2crs[jblk0] = ijcrs;
			}
			// [L] * [D^-1*U] 
			for(int ikcrs=colind[iblk];ikcrs<m_diaInd[iblk];ikcrs++){
				const int kblk = rowptr[ikcrs]; assert( kblk<nblk );
				const double* vik = &vcrs[ikcrs*blksize];
				for(int kjcrs=m_diaInd[kblk];kjcrs<colind[kblk+1];kjcrs++){
					const int jblk0 = rowptr[kjcrs]; assert( jblk0<nblk );
					double* vkj = &vcrs[kjcrs *blksize]; assert( vkj != 0 );
					double* vij = 0;
					if( jblk0 != iblk ){
						const int ijcrs0 = row2crs[jblk0];
            if( ijcrs0 == -1 ){ continue; }
						vij = &vcrs[ijcrs0*blksize];
					}
          else{ vij = &vdia[iblk *blksize]; }
					assert( vij != 0 );
          CalcSubMatPr(vij,vik,vkj, len,len,len);
				}
			}
			{
				double* vii = &vdia[iblk*blksize];
				int info = 0;
				CalcInvMat(vii,len,info);
				if( info==1 ){
					std::cout << "frac false" << iblk << std::endl;
					icnt_sing++;
					if( icnt_sing > nmax_sing ){
						delete[] pTmpBlk;
						return;
					}
				}
			}
			// [U] = [1/D][U]
      for(int ijcrs=m_diaInd[iblk];ijcrs<colind[iblk+1];ijcrs++){ assert( ijcrs<m_ncrs );
				double* vij = &vcrs[ijcrs*blksize];
				const double* pVal_ii = &vdia[iblk *blksize];
        CalcMatPr(vij,pVal_ii,pTmpBlk,  len,len);
			}
      for(int ijcrs=colind[iblk];ijcrs<colind[iblk+1];ijcrs++){ assert( ijcrs<m_ncrs );
				const int jblk0 = rowptr[ijcrs]; assert( jblk0<nblk );
				row2crs[jblk0] = -1;
			}
		}	// end iblk
		delete[] pTmpBlk;
	}  
	return;
}



void Solve_PCG
(double& conv_ratio,
 int& iteration,
 const CMatrixSquareSparse& mat,
 const CPreconditionerILU& ilu,
 std::vector<double>& r_vec,
 std::vector<double>& x_vec)
{
	const double conv_ratio_tol = conv_ratio;
	const int mx_iter = iteration;
    
	const int nblk = mat.m_nblk;
  const int len = mat.m_len;
  assert(r_vec.size() == nblk*len);
  const int ndof = nblk*len;  
  
  // {x} = 0
  x_vec.resize(ndof);
  for(int i=0;i<ndof;i++){ x_vec[i] = 0; }
  
	double inv_sqnorm_res0;
	{
		const double sqnorm_res0 = InnerProduct(r_vec,r_vec);
		if( sqnorm_res0 < 1.0e-30 ){
			conv_ratio = 0.0;
			iteration = 0;
			return;
		}
		inv_sqnorm_res0 = 1.0 / sqnorm_res0;
	}
  
  // {Pr} = [P]{r}
  std::vector<double> Pr_vec = r_vec;
  ilu.Solve(Pr_vec);
  // {p} = {Pr}
  std::vector<double> p_vec = Pr_vec;
  
  // rPr = ({r},{Pr})
	double rPr = InnerProduct(r_vec,Pr_vec);
	for(int iitr=0;iitr<mx_iter;iitr++){
    
		{
      std::vector<double>& Ap_vec = Pr_vec;      
      // {Ap} = [A]{p}
			mat.MatVec(1.0,p_vec,0.0,Ap_vec);
      // alpha = ({r},{Pr})/({p},{Ap})
			const double pAp = InnerProduct(p_vec,Ap_vec);
			double alpha = rPr / pAp;
      // {r} = -alpha*{Ap} + {r}
      AXPY(-alpha,Ap_vec,r_vec);
      // {x} = +alpha*{p } + {x}
      AXPY(+alpha,p_vec, x_vec);
    }
    
		{	// Converge Judgement
			double sqnorm_res = InnerProduct(r_vec,r_vec);
      // std::cout << iitr << " " << sqrt(sq_norm_res * sq_inv_norm_res0) << std::endl;
			if( sqnorm_res * inv_sqnorm_res0 < conv_ratio_tol*conv_ratio_tol ){
				conv_ratio = sqrt( sqnorm_res * inv_sqnorm_res0 );
				iteration = iitr;
				return;
			}
		}
    
		{	// calc beta
      // {Pr} = [P]{r}
      for(int i=0;i<ndof;i++){ Pr_vec[i] = r_vec[i]; }
			ilu.Solve(Pr_vec);
      // rPr1 = ({r},{Pr})
			const double rPr1 = InnerProduct(r_vec,Pr_vec);
      // beta = rPr1/rPr
			double beta = rPr1/rPr;
			rPr = rPr1;
      // {p} = {Pr} + beta*{p}
      for(int i=0;i<ndof;i++){ p_vec[i] = Pr_vec[i] + beta*p_vec[i]; }
    }
	}
	// Converge Judgement
  double sq_norm_res = InnerProduct(r_vec,r_vec);
  conv_ratio = sqrt( sq_norm_res * inv_sqnorm_res0 );
  return;
}


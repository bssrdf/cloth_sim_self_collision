﻿//
//  solve_internal_eigen.h
//  self_contact
//
//  Created by Nobuyuki Umetani on 11/17/13.
//  Copyright (c) 2013 Nobuyuki Umetani. All rights reserved.
//

#ifndef solve_internal_eigen_h
#define solve_internal_eigen_h

#include "matrix_square_sparse.h"
#include "ilu_sparse.h"
#include "cloth_internal_physics.h"


// compute total energy and its first and second derivatives
// 全体のエネルギーとその，節点位置における一階微分，二階微分を計算
void AddWdWddW_Cloth
(double& W, // (out) energy，歪エネルギー
 std::vector<double>& dW, // (out) first derivative of energy，歪エネルギーの一階微分
 CMatrixSquareSparse& ddW, // (out) second derivative of energy，歪エネルギーの二階微分
 std::vector<int>& tmp_buffer,
 ////
 const std::vector<double>& aXYZ, // (in) deformed vertex positions，現在の頂点の座標配列
 const std::vector<double>& aXYZ0, // (in) initial vertex positions，変形前の頂点の座標配列
 const std::vector<int>& aTri, // (in) triangle index，三角形の頂点インデックス配列
 const std::vector<int>& aQuad, // (in) index of 4 vertices required for bending，曲げ計算のための４頂点のインデックス配列
 double lambda, // (in) Lame's 1st parameter，ラメの第一定数
 double myu,  // (in) Lame's 2nd parameter　ラメの第二定数
 double stiff_bend // (in) bending stiffness，曲げ剛性
 )
{
  // marge element in-plane strain energy
  // 面内歪エネルギーを追加
  for(int itri=0;itri<aTri.size()/3;itri++){
    const int aIP[3] = { aTri[itri*3+0], aTri[itri*3+1], aTri[itri*3+2] };
    double C[3][3]; double c[3][3];
    for(int ino=0;ino<3;ino++){
      const int ip = aIP[ino];
      for(int i=0;i<3;i++){ C[ino][i] = aXYZ0[ip*3+i]; }
      for(int i=0;i<3;i++){ c[ino][i] = aXYZ [ip*3+i]; }
    }
    double e, de[3][3], dde[3][3][3][3];
    WdWddW_CST( e,de,dde, C,c, lambda,myu );
    W += e;  // marge energy
    // marge de
    for(int ino=0;ino<3;ino++){
      const int ip = aIP[ino];
      for(int i =0;i<3;i++){ dW[ip*3+i] += de[ino][i]; }
    }
    // marge dde
    ddW.Mearge(3, aIP, 3, aIP, 9, &dde[0][0][0][0], tmp_buffer);
  }
  // marge element bending energy
  // 曲げエネルギーを追加
  for(int iq=0;iq<aQuad.size()/4;iq++){
    const int aIP[4] = { aQuad[iq*4+0], aQuad[iq*4+1], aQuad[iq*4+2], aQuad[iq*4+3] };
    double C[4][3]; double c[4][3];
    for(int ino=0;ino<4;ino++){
      const int ip = aIP[ino];
      for(int i=0;i<3;i++){ C[ino][i] = aXYZ0[ip*3+i]; }
      for(int i=0;i<3;i++){ c[ino][i] = aXYZ [ip*3+i]; }
    }
    double e, de[4][3], dde[4][4][3][3];
    WdWddW_Bend( e,de,dde, C,c, stiff_bend );
    W += e;  // marge energy
    // marge de
    for(int ino=0;ino<4;ino++){
      const int ip = aIP[ino];
      for(int i =0;i<3;i++){ dW[ip*3+i] += de[ino][i]; }
    }
    // marge dde
    ddW.Mearge(4, aIP, 4, aIP, 9, &dde[0][0][0][0], tmp_buffer);
  }
}


// compute total energy and its first and second derivatives
// 全体のエネルギーとその，節点位置における一階微分，二階微分を計算
void AddWdW_Cloth
(double& W, // (out) energy，歪エネルギー
 std::vector<double>& dW, // (out) first derivative of energy，歪エネルギーの一階微分
 ////
 const std::vector<double>& aXYZ, // (in) deformed vertex positions，現在の頂点の座標配列
 const std::vector<double>& aXYZ0, // (in) initial vertex positions，変形前の頂点の座標配列
 const std::vector<int>& aTri, // (in) triangle index，三角形の頂点インデックス配列
 const std::vector<int>& aQuad, // (in) index of 4 vertices required for bending，曲げ計算のための４頂点のインデックス配列
 double lambda, // (in) Lame's 1st parameter，ラメの第一定数
 double myu,  // (in) Lame's 2nd parameter　ラメの第二定数
 double stiff_bend // (in) bending stiffness，曲げ剛性
 )
{
  // marge element in-plane strain energy
  // 面内歪エネルギーを追加
  for(int itri=0;itri<aTri.size()/3;itri++){
    const int aIP[3] = { aTri[itri*3+0], aTri[itri*3+1], aTri[itri*3+2] };
    double C[3][3]; double c[3][3];
    for(int ino=0;ino<3;ino++){
      const int ip = aIP[ino];
      for(int i=0;i<3;i++){ C[ino][i] = aXYZ0[ip*3+i]; }
      for(int i=0;i<3;i++){ c[ino][i] = aXYZ [ip*3+i]; }
    }
    double e, de[3][3], dde[3][3][3][3];
    WdWddW_CST( e,de,dde, C,c, lambda,myu );
    W += e;  // marge energy
    // marge de
    for(int ino=0;ino<3;ino++){
      const int ip = aIP[ino];
      for(int i =0;i<3;i++){ dW[ip*3+i] += de[ino][i]; }
    }
  }
  // marge element bending energy
  // 曲げエネルギーを追加
  for(int iq=0;iq<aQuad.size()/4;iq++){
    const int aIP[4] = { aQuad[iq*4+0], aQuad[iq*4+1], aQuad[iq*4+2], aQuad[iq*4+3] };
    double C[4][3]; double c[4][3];
    for(int ino=0;ino<4;ino++){
      const int ip = aIP[ino];
      for(int i=0;i<3;i++){ C[ino][i] = aXYZ0[ip*3+i]; }
      for(int i=0;i<3;i++){ c[ino][i] = aXYZ [ip*3+i]; }
    }
    double e, de[4][3], dde[4][4][3][3];
    WdWddW_Bend( e,de,dde, C,c, stiff_bend );
    W += e;  // marge energy
    // marge de
    for(int ino=0;ino<4;ino++){
      const int ip = aIP[ino];
      for(int i =0;i<3;i++){ dW[ip*3+i] += de[ino][i]; }
    }
  }
}


// compute total energy and its first and second derivatives
// 全体のエネルギーとその，節点位置における一階微分，二階微分を計算
void AddWdWddW_Contact
(double& W, // (out) energy，歪エネルギー
 std::vector<double>& dW, // (out) first derivative of energy，歪エネルギーの一階微分
 CMatrixSquareSparse& ddW, // (out) second derivative of energy，歪エネルギーの二階微分
 std::vector<int>& tmp_buffer,
 ////
 const std::vector<double>& aXYZ, // (in) deformed vertex positions，現在の頂点の座標配列
 double stiff_contact,
 double contact_clearance,
 void (*penetrationDepth)(double& , double* , const double*)
 )
{
  // marge point-wise external contact energy
  // 面内歪エネルギーを追加
  for(int ip=0;ip<aXYZ.size()/3;ip++){
    double c[3] = { aXYZ[ip*3+0], aXYZ[ip*3+1], aXYZ[ip*3+2] };
    double e, de[3], dde[3][3];
    WdWddW_Contact( e,de,dde, c, stiff_contact,contact_clearance, penetrationDepth );
    W += e;  // marge energy
    // marge de
    for(int i =0;i<3;i++){ dW[ip*3+i] += de[i]; }
    // marge dde
    ddW.Mearge(1, &ip, 1, &ip, 9, &dde[0][0], tmp_buffer);
  }
}

// compute total energy and its first and second derivatives
// 全体のエネルギーとその，節点位置における一階微分，二階微分を計算
void AddWdW_Gravity
(double& W, // (out) energy，歪エネルギー
 std::vector<double>& dW, // (out) first derivative of energy，歪エネルギーの一階微分
 ////
 const std::vector<double>& aXYZ, // (in) deformed vertex positions，現在の頂点の座標配列
 const double gravity[3], // (in) gravitational accerelation，重力加速度
 double mass_point // (in) mass of a point，頂点の質量
 )
{
  // marge potential energy
  // 重力ポテンシャル・エネルギーを追加
  for(int ip=0;ip<aXYZ.size()/3;ip++){
    const double c[3] = {aXYZ[ip*3+0],aXYZ[ip*3+1],aXYZ[ip*3+2]};
    W -= mass_point*( c[0]*gravity[0] + c[1]*gravity[1] + c[2]*gravity[2] );
    dW[ip*3+0] -= mass_point*gravity[0];
    dW[ip*3+1] -= mass_point*gravity[1];
    dW[ip*3+2] -= mass_point*gravity[2];
  }
}

void StepTime_InternalDynamics
(
 std::vector<double>& aXYZ, // (in,out) deformed vertex positions，現在の頂点位置配列
 std::vector<double>& aUVW, // (in,out) deformed vertex velocity，現在の頂点速度配列
 CMatrixSquareSparse& mat_A,
 ////
 const std::vector<double>& aXYZ0,// (in) initial vertex positions，変形前の頂点の座標配列
 const std::vector<int>& aBCFlag, // (in) boundary condition flag (0:free 1:fixed)，境界条件フラグの配列
 const std::vector<int>& aTri, // (in) triangle index，三角形の頂点インデックス配列
 const std::vector<int>& aQuad, // (in) index of 4 vertices required for bending，曲げ計算のための４頂点のインデックス配列
 const double dt, // (in) size of time step，時間ステップの大きさ
 double lambda, // (in) Lame's 1st parameter，ラメ第一定数
 double myu, // (in) Lame's 2nd parameter，ラメ第二定数
 double stiff_bend, // (in) bending stiffness 曲げ剛性
 const double gravity[3], // (in) gravitatinal accereration，重力加速度
 double mass_point, // (in) mass for a point，頂点あたりの質量
 double stiff_contact,
 double contact_clearance,
 void (*penetrationDepth)(double& , double* , const double*)
 )
{
  const int np = (int)aXYZ.size()/3; // number of point，頂点数
  const int nDof = np*3; // degree of freedom，全自由度数
  // compute total energy and its first and second derivatives
  // 全体のエネルギーとその，節点位置における一階微分，二階微分を計算
  double W = 0;
  std::vector<double> vec_b(nDof,0);
	mat_A.SetZero();
  std::vector<int> tmp_buffer(np,-1);
  AddWdWddW_Cloth(W,vec_b,mat_A,
                  tmp_buffer,
                  aXYZ,aXYZ0,
                  aTri,aQuad,
                  lambda,myu,stiff_bend);
  AddWdWddW_Contact(W,vec_b,mat_A,tmp_buffer,
                    aXYZ,
                    stiff_contact,contact_clearance,penetrationDepth);
  AddWdW_Gravity(W,vec_b,
                 aXYZ,
                 gravity,mass_point);
  std::cout << "energy : " << W << std::endl;
  // compute coefficient matrix and left-hand-side vector
  // Back-ward Eular time integration
  for(int i=0;i<nDof;i++){
    vec_b[i] = -vec_b[i] + mass_point*aUVW[i]/dt;
  }
  for(int ip=0;ip<np;ip++){
    mat_A.m_valDia[ip*9+0*3+0] += mass_point / (dt*dt);
    mat_A.m_valDia[ip*9+1*3+1] += mass_point / (dt*dt);
    mat_A.m_valDia[ip*9+2*3+2] += mass_point / (dt*dt);
  }
  mat_A.SetBoundaryCondition(aBCFlag);
  for(unsigned int ip=0;ip<np;ip++){
    if( aBCFlag[ip] == 0 ) continue;
    vec_b[ip*3+0] = 0;
    vec_b[ip*3+1] = 0;
    vec_b[ip*3+2] = 0;
  }
  // solve linear system，連立一次方程式を解く
  std::vector<double> vec_x;
  double conv_ratio = 1.0e-4;
  int iteration = 1000;
  Solve_CG(conv_ratio,iteration,mat_A,vec_b,vec_x);
  std::cout << "  conv_ratio:" << conv_ratio << "  iteration:" << iteration << std::endl;
  // update position，頂点位置の更新
  for(int i=0;i<nDof;i++){ aXYZ[i] += vec_x[i]; }
  // update velocity，頂点の速度の更新
  for(int i=0;i<nDof;i++){ aUVW[i] = vec_x[i]/dt; }
}

void StepTime_InternalDynamicsILU
(
 std::vector<double>& aXYZ, // (in,out) deformed vertex positions，現在の頂点位置配列
 std::vector<double>& aUVW, // (in,out) deformed vertex velocity，現在の頂点速度配列
 CMatrixSquareSparse& mat_A,
 CPreconditionerILU& ilu_A,
 ////
 const std::vector<double>& aXYZ0,// (in) initial vertex positions，変形前の頂点の座標配列
 const std::vector<int>& aBCFlag, // (in) boundary condition flag (0:free 1:fixed)，境界条件フラグの配列
 const std::vector<int>& aTri, // (in) triangle index，三角形の頂点インデックス配列
 const std::vector<int>& aQuad, // (in) index of 4 vertices required for bending，曲げ計算のための４頂点のインデックス配列
 const double dt, // (in) size of time step，時間ステップの大きさ
 double lambda, // (in) Lame's 1st parameter，ラメ第一定数
 double myu, // (in) Lame's 2nd parameter，ラメ第二定数
 double stiff_bend, // (in) bending stiffness 曲げ剛性
 const double gravity[3], // (in) gravitatinal accereration，重力加速度
 double mass_point, // (in) mass for a point，頂点あたりの質量
 double stiff_contact,
 double contact_clearance,
 void (*penetrationDepth)(double& , double* , const double*)
 )
{
  const int np = (int)aXYZ.size()/3; // number of point，頂点数
  const int nDof = np*3; // degree of freedom，全自由度数
  // compute total energy and its first and second derivatives
  // 全体のエネルギーとその，節点位置における一階微分，二階微分を計算
  double W = 0;
  std::vector<double> vec_b(nDof,0);
	mat_A.SetZero();
  std::vector<int> tmp_buffer(np,-1);
  AddWdWddW_Cloth(W,vec_b,mat_A,
                  tmp_buffer,
                  aXYZ,aXYZ0,
                  aTri,aQuad,
                  lambda,myu,stiff_bend);
  AddWdWddW_Contact(W,vec_b,mat_A,tmp_buffer,
                    aXYZ,
                    stiff_contact,contact_clearance,penetrationDepth);
  AddWdW_Gravity(W,vec_b,
                 aXYZ,
                 gravity,mass_point);
  std::cout << "energy : " << W << std::endl;
  // compute coefficient matrix and left-hand-side vector
  // Back-ward Eular time integration
  for(int i=0;i<nDof;i++){
    vec_b[i] = -vec_b[i] + mass_point*aUVW[i]/dt;
  }
  for(int ip=0;ip<np;ip++){
    mat_A.m_valDia[ip*9+0*3+0] += mass_point / (dt*dt);
    mat_A.m_valDia[ip*9+1*3+1] += mass_point / (dt*dt);
    mat_A.m_valDia[ip*9+2*3+2] += mass_point / (dt*dt);
  }
  mat_A.SetBoundaryCondition(aBCFlag);
  for(unsigned int ip=0;ip<np;ip++){
    if( aBCFlag[ip] == 0 ) continue;
    vec_b[ip*3+0] = 0;
    vec_b[ip*3+1] = 0;
    vec_b[ip*3+2] = 0;
  }
  ilu_A.SetValueILU(mat_A);
  ilu_A.DoILUDecomp();
  // solve linear system，連立一次方程式を解く
  std::vector<double> vec_x;
  double conv_ratio = 1.0e-4;
  int iteration = 100;
  Solve_PCG(conv_ratio, iteration, mat_A,ilu_A, vec_b,vec_x);
  std::cout << "  conv_ratio:" << conv_ratio << "  iteration:" << iteration << std::endl;
  // update position，頂点位置の更新
  for(int i=0;i<nDof;i++){ aXYZ[i] += vec_x[i]; }
  // update velocity，頂点の速度の更新
  for(int i=0;i<nDof;i++){ aUVW[i] = vec_x[i]/dt; }
}


void UpdateIntermidiateVelocity
(std::vector<double>& aUVW, // (in,out) deformed vertex velocity，現在の頂点速度配列
 ////
 const std::vector<double>& aXYZ, // (in,out) deformed vertex positions，現在の頂点位置配列
 const std::vector<double>& aXYZ0,// (in) initial vertex positions，変形前の頂点の座標配列
 const std::vector<int>& aBCFlag, // (in) boundary condition flag (0:free 1:fixed)，境界条件フラグの配列
 const std::vector<int>& aTri, // (in) triangle index，三角形の頂点インデックス配列
 const std::vector<int>& aQuad, // (in) index of 4 vertices required for bending，曲げ計算のための４頂点のインデックス配列
 const double dt, // (in) size of time step，時間ステップの大きさ
 double lambda, // (in) Lame's 1st parameter，ラメ第一定数
 double myu, // (in) Lame's 2nd parameter，ラメ第二定数
 double stiff_bend, // (in) bending stiffness 曲げ剛性
 const double gravity[3], // (in) gravitatinal accereration，重力加速度
 double mass_point // (in) mass for a point，頂点あたりの質量
 )
{
  const int np = (int)aXYZ.size()/3; // number of point，頂点数
  const int nDof = np*3; // degree of freedom，全自由度数
  // compute total energy and its first and second derivatives
  // 全体のエネルギーとその，節点位置における一階微分，二階微分を計算
  double W = 0;
  std::vector<double> dW(nDof,0);
  AddWdW_Cloth(W,dW,
               aXYZ,aXYZ0,
               aTri,aQuad,
               lambda,myu,stiff_bend);
  AddWdW_Gravity(W,dW,
                 aXYZ,
                 gravity,mass_point);
  for(int ip=0;ip<np;ip++){
    if( aBCFlag[ip] == 0 ) continue;
    aUVW[ip*3+0] = 0;
    aUVW[ip*3+1] = 0;
    aUVW[ip*3+2] = 0;
  }
  for(unsigned int ip=0;ip<np;ip++){
    aUVW[ip*3+0] += -(0.5*dt/mass_point)*dW[ip*3+0];
    aUVW[ip*3+1] += -(0.5*dt/mass_point)*dW[ip*3+1];
    aUVW[ip*3+2] += -(0.5*dt/mass_point)*dW[ip*3+2];
  }
}


#endif

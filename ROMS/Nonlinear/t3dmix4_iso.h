#ifdef EW_PERIODIC
# define I_RANGE Istr-1,Iend+1
#else
# define I_RANGE MAX(Istr-1,1),MIN(Iend+1,Lm(ng))
#endif
#ifdef NS_PERIODIC
# define J_RANGE Jstr-1,Jend+1
#else
# define J_RANGE MAX(Jstr-1,1),MIN(Jend+1,Mm(ng))
#endif

      SUBROUTINE t3dmix4 (ng, tile)
!
!svn $Id: t3dmix4_iso.h 553 2011-04-22 21:30:04Z arango $
!***********************************************************************
!  Copyright (c) 2002-2011 The ROMS/TOMS Group                         !
!    Licensed under a MIT/X style license                              !
!    See License_ROMS.txt                           Hernan G. Arango   !
!****************************************** Alexander F. Shchepetkin ***
!                                                                      !
!  This subroutine computes horizontal biharmonic mixing of tracers    !
!  along isopycnic surfaces.                                           !
!                                                                      !
!***********************************************************************
!
      USE mod_param
#ifdef CLIMA_TS_MIX
      USE mod_clima
#endif
#ifdef DIAGNOSTICS_TS
      USE mod_diags
#endif
      USE mod_grid
      USE mod_mixing
      USE mod_ocean
      USE mod_stepping
!
!  Imported variable declarations.
!
      integer, intent(in) :: ng, tile
!
!  Local variable declarations.
!
#include "tile.h"
!
#ifdef PROFILE
      CALL wclock_on (ng, iNLM, 29)
#endif
      CALL t3dmix4_tile (ng, tile,                                      &
     &                   LBi, UBi, LBj, UBj,                            &
     &                   IminS, ImaxS, JminS, JmaxS,                    &
     &                   nrhs(ng), nnew(ng),                            &
#ifdef MASKING
     &                   GRID(ng) % umask,                              &
     &                   GRID(ng) % vmask,                              &
#endif
     &                   GRID(ng) % om_v,                               &
     &                   GRID(ng) % on_u,                               &
     &                   GRID(ng) % pm,                                 &
     &                   GRID(ng) % pn,                                 &
     &                   GRID(ng) % Hz,                                 &
     &                   GRID(ng) % z_r,                                &
#ifdef DIFF_3DCOEF
# ifdef TS_U3ADV_SPLIT
     &                   MIXING(ng) % diff3d_u,                         &
     &                   MIXING(ng) % diff3d_v,                         &
# else
     &                   MIXING(ng) % diff3d_r,                         &
# endif
#else
     &                   MIXING(ng) % diff4,                            &
#endif
     &                   OCEAN(ng) % rho,                               &
#ifdef CLIMA_TS_MIX
     &                   CLIMA(ng) % tclm,                              &
#endif
#ifdef DIAGNOSTICS_TS
     &                   DIAGS(ng) % DiaTwrk,                           &
#endif
     &                   OCEAN(ng) % t)
#ifdef PROFILE
      CALL wclock_off (ng, iNLM, 29)
#endif
      RETURN
      END SUBROUTINE t3dmix4
!
!***********************************************************************
      SUBROUTINE t3dmix4_tile (ng, tile,                                &
     &                         LBi, UBi, LBj, UBj,                      &
     &                         IminS, ImaxS, JminS, JmaxS,              &
     &                         nrhs, nnew,                              &
#ifdef MASKING
     &                         umask, vmask,                            &
#endif
     &                         om_v, on_u, pm, pn,                      &
     &                         Hz, z_r,                                 &
#ifdef DIFF_3DCOEF
# ifdef TS_U3ADV_SPLIT
     &                         diff3d_u, diff3d_v,                      &
# else
     &                         diff3d_r,                                &
# endif
#else
     &                         diff4,                                   &
#endif
     &                         rho,                                     &
#ifdef CLIMA_TS_MIX
     &                         tclm,                                    &
#endif
#ifdef DIAGNOSTICS_TS
     &                         DiaTwrk,                                 &
#endif
     &                         t)
!***********************************************************************
!
      USE mod_param
      USE mod_scalars
!
!  Imported variable declarations.
!
      integer, intent(in) :: ng, tile
      integer, intent(in) :: LBi, UBi, LBj, UBj
      integer, intent(in) :: IminS, ImaxS, JminS, JmaxS
      integer, intent(in) :: nrhs, nnew

#ifdef ASSUMED_SHAPE
# ifdef MASKING
      real(r8), intent(in) :: umask(LBi:,LBj:)
      real(r8), intent(in) :: vmask(LBi:,LBj:)
# endif
# ifdef DIFF_3DCOEF
#  ifdef TS_U3ADV_SPLIT
      real(r8), intent(in) :: diff3d_u(LBi:,LBj:,:)
      real(r8), intent(in) :: diff3d_v(LBi:,LBj:,:)
#  else
      real(r8), intent(in) :: diff3d_r(LBi:,LBj:,:)
#  endif
# else
      real(r8), intent(in) :: diff4(LBi:,LBj:,:)
# endif
      real(r8), intent(in) :: om_v(LBi:,LBj:)
      real(r8), intent(in) :: on_u(LBi:,LBj:)
      real(r8), intent(in) :: pm(LBi:,LBj:)
      real(r8), intent(in) :: pn(LBi:,LBj:)
      real(r8), intent(in) :: Hz(LBi:,LBj:,:)
      real(r8), intent(in) :: z_r(LBi:,LBj:,:)
      real(r8), intent(in) :: rho(LBi:,LBj:,:)
# ifdef CLIMA_TS_MIX
      real(r8), intent(in) :: tclm(LBi:,LBj:,:,:)
# endif
# ifdef DIAGNOSTICS_TS
      real(r8), intent(inout) :: DiaTwrk(LBi:,LBj:,:,:,:)
# endif
      real(r8), intent(inout) :: t(LBi:,LBj:,:,:,:)
#else
# ifdef MASKING
      real(r8), intent(in) :: umask(LBi:UBi,LBj:UBj)
      real(r8), intent(in) :: vmask(LBi:UBi,LBj:UBj)
# endif
# ifdef DIFF_3DCOEF
#  ifdef TS_U3ADV_SPLIT
      real(r8), intent(in) :: diff3d_u(LBi:UBi,LBj:UBj,N(ng))
      real(r8), intent(in) :: diff3d_v(LBi:UBi,LBj:UBj,N(ng))
#  else
      real(r8), intent(in) :: diff3d_r(LBi:UBi,LBj:UBj,N(ng))
#  endif
# else
      real(r8), intent(in) :: diff4(LBi:UBi,LBj:UBj,NT(ng))
# endif
      real(r8), intent(in) :: om_v(LBi:UBi,LBj:UBj)
      real(r8), intent(in) :: on_u(LBi:UBi,LBj:UBj)
      real(r8), intent(in) :: pm(LBi:UBi,LBj:UBj)
      real(r8), intent(in) :: pn(LBi:UBi,LBj:UBj)
      real(r8), intent(in) :: Hz(LBi:UBi,LBj:UBj,N(ng))
      real(r8), intent(in) :: z_r(LBi:UBi,LBj:UBj,N(ng))
      real(r8), intent(in) :: rho(LBi:UBi,LBj:UBj,N(ng))
# ifdef CLIMA_TS_MIX
      real(r8), intent(in) :: tclm(LBi:UBi,LBj:UBj,N(ng),NT(ng))
# endif
# ifdef DIAGNOSTICS_TS
      real(r8), intent(inout) :: DiaTwrk(LBi:UBi,LBj:UBj,N(ng),NT(ng),  &
     &                                   NDT)
# endif
      real(r8), intent(inout) :: t(LBi:UBi,LBj:UBj,N(ng),3,NT(ng))
#endif
!
!  Local variable declarations.
!
      integer :: i, itrc, j, k, k1, k2

      real(r8), parameter :: eps = 0.5_r8
      real(r8), parameter :: small = 1.0E-14_r8
      real(r8), parameter :: slope_max = 0.0001_r8
      real(r8), parameter :: strat_min = 0.1_r8

      real(r8) :: cff, cff1, cff2, cff3, cff4, cff5, fac

      real(r8), dimension(IminS:ImaxS,JminS:JmaxS,N(ng)) :: LapT

      real(r8), dimension(IminS:ImaxS,JminS:JmaxS) :: FE
      real(r8), dimension(IminS:ImaxS,JminS:JmaxS) :: FX

      real(r8), dimension(IminS:ImaxS,JminS:JmaxS,2) :: FS
      real(r8), dimension(IminS:ImaxS,JminS:JmaxS,2) :: dRde
      real(r8), dimension(IminS:ImaxS,JminS:JmaxS,2) :: dRdx
      real(r8), dimension(IminS:ImaxS,JminS:JmaxS,2) :: dTde
      real(r8), dimension(IminS:ImaxS,JminS:JmaxS,2) :: dTdr
      real(r8), dimension(IminS:ImaxS,JminS:JmaxS,2) :: dTdx

#include "set_bounds.h"
!
!-----------------------------------------------------------------------
!  Compute horizontal biharmonic diffusion along isopycnic surfaces.
!  The biharmonic operator is computed by applying the harmonic
!  operator twice.
!-----------------------------------------------------------------------
!
!  Compute horizontal and density gradients.  Notice the recursive
!  blocking sequence.  The vertical placement of the gradients is:
!
!        dTdx,dTde(:,:,k1) k     rho-points
!        dTdx,dTde(:,:,k2) k+1   rho-points
!          FS,dTdr(:,:,k1) k-1/2   W-points
!          FS,dTdr(:,:,k2) k+1/2   W-points
!
      T_LOOP : DO itrc=1,NT(ng)
        k2=1
        K_LOOP1 : DO k=0,N(ng)
          k1=k2
          k2=3-k1
          IF (k.lt.N(ng)) THEN
            DO j=J_RANGE
              DO i=I_RANGE+1
                cff=0.5_r8*(pm(i,j)+pm(i-1,j))
#ifdef MASKING
                cff=cff*umask(i,j)
#endif
                dRdx(i,j,k2)=cff*(rho(i  ,j,k+1)-                       &
     &                            rho(i-1,j,k+1))
#ifdef CLIMA_TS_MIX
                dTdx(i,j,k2)=cff*((t(i  ,j,k+1,nrhs,itrc)-              &
     &                             tclm(i  ,j,k+1,itrc))-               &
     &                            (t(i-1,j,k+1,nrhs,itrc)-              &
     &                             tclm(i-1,j,k+1,itrc)))
#else
                dTdx(i,j,k2)=cff*(t(i  ,j,k+1,nrhs,itrc)-               &
     &                            t(i-1,j,k+1,nrhs,itrc))
#endif
              END DO
            END DO
            DO j=J_RANGE+1
              DO i=I_RANGE
                cff=0.5_r8*(pn(i,j)+pn(i,j-1))
#ifdef MASKING
                cff=cff*vmask(i,j)
#endif
                dRde(i,j,k2)=cff*(rho(i,j  ,k+1)-                       &
     &                            rho(i,j-1,k+1))
#ifdef CLIMA_TS_MIX
                dTde(i,j,k2)=cff*((t(i,j  ,k+1,nrhs,itrc)-              &
     &                             tclm(i,j  ,k+1,itrc))-               &
     &                            (t(i,j-1,k+1,nrhs,itrc)-              &
     &                             tclm(i,j-1,k+1,itrc)))
#else
                dTde(i,j,k2)=cff*(t(i,j  ,k+1,nrhs,itrc)-               &
     &                            t(i,j-1,k+1,nrhs,itrc))
#endif
              END DO
            END DO
          END IF
          IF ((k.eq.0).or.(k.eq.N(ng))) THEN
            DO j=-1+J_RANGE+1
              DO i=-1+I_RANGE+1
                dTdr(i,j,k2)=0.0_r8
                FS(i,j,k2)=0.0_r8
              END DO
            END DO
          ELSE
            DO j=-1+J_RANGE+1
              DO i=-1+I_RANGE+1
#if defined MAX_SLOPE
                cff1=SQRT(dRdx(i,j,k2)**2+dRdx(i+1,j,k2)**2+            &
     &                    dRdx(i,j,k1)**2+dRdx(i+1,j,k1)**2+            &
     &                    dRde(i,j,k2)**2+dRde(i,j+1,k2)**2+            &
     &                    dRde(i,j,k1)**2+dRde(i,j+1,k1)**2)
                cff2=0.25_r8*slope_max*                                 &
     &               (z_r(i,j,k+1)-z_r(i,j,k))*cff1
                cff3=MAX(rho(i,j,k)-rho(i,j,k+1),small)
                cff4=MAX(cff2,cff3)
                cff=-1.0_r8/cff4
#elif defined MIN_STRAT
                cff1=MAX(rho(i,j,k)-rho(i,j,k+1),                       &
     &                   strat_min*(z_r(i,j,k+1)-z_r(i,j,k)))
                cff=-1.0_r8/cff1
#else
                cff1=MAX(rho(i,j,k)-rho(i,j,k+1),eps)
                cff=-1.0_r8/cff1
#endif
#ifdef CLIMA_TS_MIX
                dTdr(i,j,k2)=cff*((t(i,j,k+1,nrhs,itrc)-                &
     &                             tclm(i,j,k+1,itrc))-                 &
     &                            (t(i,j,k  ,nrhs,itrc)-                &
     &                             tclm(i,j,k  ,itrc)))
#else
                dTdr(i,j,k2)=cff*(t(i,j,k+1,nrhs,itrc)-                 &
     &                            t(i,j,k  ,nrhs,itrc))
#endif
                FS(i,j,k2)=cff*(z_r(i,j,k+1)-                           &
     &                          z_r(i,j,k  ))
              END DO
            END DO
          END IF
          IF (k.gt.0) THEN
            DO j=J_RANGE
              DO i=I_RANGE+1
#ifdef DIFF_3DCOEF
# ifdef TS_U3ADV_SPLIT
                cff=0.5_r8*diff3d_u(i,j,k)*on_u(i,j)
# else
                cff=0.25_r8*(diff3d_r(i,j,k)+diff3d_r(i-1,j,k))*        &
     &              on_u(i,j)
# endif
#else
                cff=0.25_r8*(diff4(i,j,itrc)+diff4(i-1,j,itrc))*        &
     &              on_u(i,j)
#endif
                FX(i,j)=cff*                                            &
     &                  (Hz(i,j,k)+Hz(i-1,j,k))*                        &
     &                  (dTdx(i,j,k1)-                                  &
     &                   0.5_r8*(MAX(dRdx(i,j,k1),0.0_r8)*              &
     &                              (dTdr(i-1,j,k1)+                    &
     &                               dTdr(i  ,j,k2))+                   &
     &                           MIN(dRdx(i,j,k1),0.0_r8)*              &
     &                              (dTdr(i-1,j,k2)+                    &
     &                               dTdr(i  ,j,k1))))
              END DO
            END DO
            DO j=J_RANGE+1
              DO i=I_RANGE
#ifdef DIFF_3DCOEF
# ifdef TS_U3ADV_SPLIT
                cff=0.5_r8*diff3d_v(i,j,k)*om_v(i,j)
# else
                cff=0.25_r8*(diff3d_r(i,j,k)+diff3d_r(i,j-1,k))*        &
     &              om_v(i,j)
# endif
#else
                cff=0.25_r8*(diff4(i,j,itrc)+diff4(i,j-1,itrc))*        &
     &              om_v(i,j)
#endif
                FE(i,j)=cff*                                            &
     &                  (Hz(i,j,k)+Hz(i,j-1,k))*                        &
     &                  (dTde(i,j,k1)-                                  &
     &                   0.5_r8*(MAX(dRde(i,j,k1),0.0_r8)*              &
     &                              (dTdr(i,j-1,k1)+                    &
     &                               dTdr(i,j  ,k2))+                   &
     &                           MIN(dRde(i,j,k1),0.0_r8)*              &
     &                              (dTdr(i,j-1,k2)+                    &
     &                               dTdr(i,j  ,k1))))
              END DO
            END DO
            IF (k.lt.N(ng)) THEN
              DO j=J_RANGE
                DO i=I_RANGE
#ifdef DIFF_3DCOEF
# ifdef TS_U3ADV_SPLIT
                  fac=0.125_r8*(diff3d_u(i,j,k  )+diff3d_u(i+1,j,k  )+  &
     &                          diff3d_u(i,j,k+1)+diff3d_u(i+1,j,k+1))
# else
                  fac=0.5_r8*diff3d_r(i,j,k)
# endif
#else
                  fac=0.5_r8*diff4(i,j,itrc)
#endif
                  cff1=MAX(dRdx(i  ,j,k1),0.0_r8)
                  cff2=MAX(dRdx(i+1,j,k2),0.0_r8)
                  cff3=MIN(dRdx(i  ,j,k2),0.0_r8)
                  cff4=MIN(dRdx(i+1,j,k1),0.0_r8)
                  cff=fac*                                              &
     &                cff1*(cff1*dTdr(i,j,k2)-dTdx(i  ,j,k1))+          &
     &                cff2*(cff2*dTdr(i,j,k2)-dTdx(i+1,j,k2))+          &
     &                cff3*(cff3*dTdr(i,j,k2)-dTdx(i  ,j,k2))+          &
     &                cff4*(cff4*dTdr(i,j,k2)-dTdx(i+1,j,k1))
                  cff1=MAX(dRde(i,j  ,k1),0.0_r8)
                  cff2=MAX(dRde(i,j+1,k2),0.0_r8)
                  cff3=MIN(dRde(i,j  ,k2),0.0_r8)
                  cff4=MIN(dRde(i,j+1,k1),0.0_r8)
#ifdef DIFF_3DCOEF
# ifdef TS_U3ADV_SPLIT
                  fac=0.125_r8*(diff3d_v(i,j,k  )+diff3d_v(i,j+1,k  )+  &
     &                          diff3d_v(i,j,k+1)+diff3d_v(i,j+1,k+1))
# else
                  fac=0.5_r8*diff3d_r(i,j,k)
# endif
#else
                  fac=0.5_r8*diff4(i,j,itrc)
#endif
                  cff=cff+                                              &
     &                fac*                                              &
     &                cff1*(cff1*dTdr(i,j,k2)-dTde(i,j  ,k1))+          &
     &                cff2*(cff2*dTdr(i,j,k2)-dTde(i,j+1,k2))+          &
     &                cff3*(cff3*dTdr(i,j,k2)-dTde(i,j  ,k2))+          &
     &                cff4*(cff4*dTdr(i,j,k2)-dTde(i,j+1,k1))
                  FS(i,j,k2)=cff*FS(i,j,k2)
                END DO
              END DO
            END IF
!
!  Compute first harmonic operator, without mixing coefficient.
!  Multiply by the metrics of the second harmonic operator.  Save
!  into work array "LapT".
!
            DO j=J_RANGE
              DO i=I_RANGE
                cff=pm(i,j)*pn(i,j)
                cff1=1.0_r8/Hz(i,j,k)
                LapT(i,j,k)=cff1*(cff*                                  &
     &                            (FX(i+1,j)-FX(i,j)+                   &
     &                             FE(i,j+1)-FE(i,j))+                  &
     &                            (FS(i,j,k2)-FS(i,j,k1)))
              END DO
            END DO
          END IF
        END DO K_LOOP1
!
!  Apply boundary conditions (except periodic; closed or gradient)
!  to the first harmonic operator.
!
#ifndef EW_PERIODIC
        IF (DOMAIN(ng)%Western_Edge(tile)) THEN
          DO k=1,N(ng)
            DO j=J_RANGE
# ifdef WESTERN_WALL
              LapT(Istr-1,j,k)=0.0_r8
# else
              LapT(Istr-1,j,k)=LapT(Istr,j,k)
# endif
            END DO
          END DO
        END IF
        IF (DOMAIN(ng)%Eastern_Edge(tile)) THEN
          DO k=1,N(ng)
            DO j=J_RANGE
# ifdef EASTERN_WALL
              LapT(Iend+1,j,k)=0.0_r8
# else
              LapT(Iend+1,j,k)=LapT(Iend,j,k)
# endif
            END DO
          END DO
        END IF
#endif
#ifndef NS_PERIODIC
        IF (DOMAIN(ng)%Southern_Edge(tile)) THEN
          DO k=1,N(ng)
            DO i=I_RANGE
# ifdef SOUTHERN_WALL
              LapT(i,Jstr-1,k)=0.0_r8
# else
              LapT(i,Jstr-1,k)=LapT(i,Jstr,k)
# endif
            END DO
          END DO
        END IF
        IF (DOMAIN(ng)%Northern_Edge(tile)) THEN
          DO k=1,N(ng)
            DO i=I_RANGE
# ifdef NORTHERN_WALL
              LapT(i,Jend+1,k)=0.0_r8
# else
              LapT(i,Jend+1,k)=LapT(i,Jend,k)
# endif
            END DO
          END DO
        END IF
#endif
#if !defined EW_PERIODIC && !defined NS_PERIODIC
        IF (DOMAIN(ng)%SouthWest_Corner(tile)) THEN
          DO k=1,N(ng)
            LapT(Istr-1,Jstr-1,k)=0.5_r8*(LapT(Istr  ,Jstr-1,k)+        &
     &                                    LapT(Istr-1,Jstr  ,k))
          END DO
        END IF
        IF (DOMAIN(ng)%SouthEast_Corner(tile)) THEN
          DO k=1,N(ng)
            LapT(Iend+1,Jstr-1,k)=0.5_r8*(LapT(Iend  ,Jstr-1,k)+        &
     &                                    LapT(Iend+1,Jstr  ,k))
          END DO
        END IF
        IF (DOMAIN(ng)%NorthWest_Corner(tile)) THEN
          DO k=1,N(ng)
            LapT(Istr-1,Jend+1,k)=0.5_r8*(LapT(Istr  ,Jend+1,k)+        &
     &                                    LapT(Istr-1,Jend  ,k))
          END DO
        END IF
        IF (DOMAIN(ng)%NorthEast_Corner(tile)) THEN
          DO k=1,N(ng)
            LapT(Iend+1,Jend+1,k)=0.5_r8*(LapT(Iend  ,Jend+1,k)+        &
     &                                    LapT(Iend+1,Jend  ,k))
          END DO
        END IF
#endif
!
!  Compute horizontal and density gradients associated with the
!  second rotated harmonic operator.
!
        k2=1
        K_LOOP2: DO k=0,N(ng)
          k1=k2
          k2=3-k1
          IF (k.lt.N(ng)) THEN
            DO j=Jstr,Jend
              DO i=Istr,Iend+1
                cff=0.5_r8*(pm(i,j)+pm(i-1,j))
#ifdef MASKING
                cff=cff*umask(i,j)
#endif
                dRdx(i,j,k2)=cff*(rho(i  ,j,k+1)-                       &
     &                            rho(i-1,j,k+1))
                dTdx(i,j,k2)=cff*(LapT(i  ,j,k+1)-                      &
     &                            LapT(i-1,j,k+1))
              END DO
            END DO
            DO j=Jstr,Jend+1
              DO i=Istr,Iend
                cff=0.5_r8*(pn(i,j)+pn(i,j-1))
#ifdef MASKING
                cff=cff*vmask(i,j)
#endif
                dRde(i,j,k2)=cff*(rho(i,j  ,k+1)-                       &
     &                            rho(i,j-1,k+1))
                dTde(i,j,k2)=cff*(LapT(i,j  ,k+1)-                      &
     &                            LapT(i,j-1,k+1))
              END DO
            END DO
          END IF
          IF ((k.eq.0).or.(k.eq.N(ng))) THEN
            DO j=Jstr-1,Jend+1
              DO i=Istr-1,Iend+1
                dTdr(i,j,k2)=0.0_r8
                FS(i,j,k2)=0.0_r8
              END DO
            END DO
          ELSE
            DO j=Jstr-1,Jend+1
              DO i=Istr-1,Iend+1
#if defined MAX_SLOPE
                cff1=SQRT(dRdx(i,j,k2)**2+dRdx(i+1,j,k2)**2+            &
     &                    dRdx(i,j,k1)**2+dRdx(i+1,j,k1)**2+            &
     &                    dRde(i,j,k2)**2+dRde(i,j+1,k2)**2+            &
     &                    dRde(i,j,k1)**2+dRde(i,j+1,k1)**2)
                cff2=0.25_r8*slope_max*                                 &
     &               (z_r(i,j,k+1)-z_r(i,j,k))*cff1
                cff3=MAX(rho(i,j,k)-rho(i,j,k+1),small)
                cff4=MAX(cff2,cff3)
                cff=-1.0_r8/cff4
#elif defined MIN_STRAT
                cff1=MAX(rho(i,j,k)-rho(i,j,k+1),                       &
     &                   strat_min*(z_r(i,j,k+1)-z_r(i,j,k)))
                cff=-1.0_r8/cff1
#else
                cff1=MAX(rho(i,j,k)-rho(i,j,k+1),eps)
                cff=-1.0_r8/cff1
#endif
                dTdr(i,j,k2)=cff*(LapT(i,j,k+1)-                        &
     &                            LapT(i,j,k  ))
                FS(i,j,k2)=cff*(z_r(i,j,k+1)-                           &
     &                          z_r(i,j,k  ))
              END DO
            END DO
          END IF
!
!  Compute components of the rotated tracer flux (T m4/s) along
!  isopycnic surfaces.
!
          IF (k.gt.0) THEN
            DO j=Jstr,Jend
              DO i=Istr,Iend+1
#ifdef DIFF_3DCOEF
# ifdef TS_U3ADV_SPLIT
                cff=0.5_r8*diff3d_u(i,j,k)*on_u(i,j)
# else
                cff=0.25_r8*(diff3d_r(i,j,k)+diff3d_r(i-1,j,k))*        &
     &              on_u(i,j)
# endif
#else
                cff=0.25_r8*(diff4(i,j,itrc)+diff4(i-1,j,itrc))*        &
     &              on_u(i,j)
#endif
                FX(i,j)=cff*                                            &
     &                  (Hz(i,j,k)+Hz(i-1,j,k))*                        &
     &                  (dTdx(i,j,k1)-                                  &
     &                   0.5_r8*(MAX(dRdx(i,j,k1),0.0_r8)*              &
     &                              (dTdr(i-1,j,k1)+                    &
     &                               dTdr(i  ,j,k2))+                   &
     &                           MIN(dRdx(i,j,k1),0.0_r8)*              &
     &                              (dTdr(i-1,j,k2)+                    &
     &                               dTdr(i  ,j,k1))))
              END DO
            END DO
            DO j=Jstr,Jend+1
              DO i=Istr,Iend
#ifdef DIFF_3DCOEF
# ifdef TS_U3ADV_SPLIT
                cff=0.5_r8*diff3d_v(i,j,k)*om_v(i,j)
# else
                cff=0.25_r8*(diff3d_r(i,j,k)+diff3d_r(i,j-1,k))*        &
     &              om_v(i,j)
# endif
#else
                cff=0.25_r8*(diff4(i,j,itrc)+diff4(i,j-1,itrc))*        &
     &              om_v(i,j)
#endif
                FE(i,j)=cff*                                            &
     &                  (Hz(i,j,k)+Hz(i,j-1,k))*                        &
     &                  (dTde(i,j,k1)-                                  &
     &                   0.5_r8*(MAX(dRde(i,j,k1),0.0_r8)*              &
     &                              (dTdr(i,j-1,k1)+                    &
     &                               dTdr(i,j  ,k2))+                   &
     &                           MIN(dRde(i,j,k1),0.0_r8)*              &
     &                              (dTdr(i,j-1,k2)+                    &
     &                               dTdr(i,j  ,k1))))
              END DO
            END DO
            IF (k.lt.N(ng)) THEN
              DO j=Jstr,Jend
                DO i=Istr,Iend
#ifdef DIFF_3DCOEF
# ifdef TS_U3ADV_SPLIT
                  fac=0.125_r8*(diff3d_u(i,j,k  )+diff3d_u(i+1,j,k  )+  &
     &                          diff3d_u(i,j,k+1)+diff3d_u(i+1,j,k+1))
# else
                  fac=0.5_r8*diff3d_r(i,j,k)
# endif
#else
                  fac=0.5_r8*diff4(i,j,itrc)
#endif
                  cff1=MAX(dRdx(i  ,j,k1),0.0_r8)
                  cff2=MAX(dRdx(i+1,j,k2),0.0_r8)
                  cff3=MIN(dRdx(i  ,j,k2),0.0_r8)
                  cff4=MIN(dRdx(i+1,j,k1),0.0_r8)
                  cff=fac*                                              &
     &                cff1*(cff1*dTdr(i,j,k2)-dTdx(i  ,j,k1))+          &
     &                cff2*(cff2*dTdr(i,j,k2)-dTdx(i+1,j,k2))+          &
     &                cff3*(cff3*dTdr(i,j,k2)-dTdx(i  ,j,k2))+          &
     &                cff4*(cff4*dTdr(i,j,k2)-dTdx(i+1,j,k1))
#ifdef DIFF_3DCOEF
# ifdef TS_U3ADV_SPLIT
                  fac=0.125_r8*(diff3d_v(i,j,k  )+diff3d_v(i,j+1,k  )+  &
     &                          diff3d_v(i,j,k+1)+diff3d_v(i,j+1,k+1))
# else
                  fac=0.5_r8*diff3d_r(i,j,k)
# endif
#else
                  fac=0.5_r8*diff4(i,j,itrc)
#endif
                  cff1=MAX(dRde(i,j  ,k1),0.0_r8)
                  cff2=MAX(dRde(i,j+1,k2),0.0_r8)
                  cff3=MIN(dRde(i,j  ,k2),0.0_r8)
                  cff4=MIN(dRde(i,j+1,k1),0.0_r8)
                  cff=cff+                                              &
     &                fac*                                              &
     &                cff1*(cff1*dTdr(i,j,k2)-dTde(i,j  ,k1))+          &
     &                cff2*(cff2*dTdr(i,j,k2)-dTde(i,j+1,k2))+          &
     &                cff3*(cff3*dTdr(i,j,k2)-dTde(i,j  ,k2))+          &
     &                cff4*(cff4*dTdr(i,j,k2)-dTde(i,j+1,k1))
                  FS(i,j,k2)=cff*FS(i,j,k2)
                END DO
              END DO
            END IF
!
! Time-step biharmonic, isopycnal diffusion term (m Tunits).
!
            DO j=Jstr,Jend
              DO i=Istr,Iend
                cff=dt(ng)*pm(i,j)*pn(i,j)
                cff1=cff*(FX(i+1,j  )-FX(i,j))
                cff2=cff*(FE(i  ,j+1)-FE(i,j))
                cff3=dt(ng)*(FS(i,j,k2)-FS(i,j,k1))
                cff4=cff1+cff2+cff3
                t(i,j,k,nnew,itrc)=t(i,j,k,nnew,itrc)-cff4
#ifdef TS_MPDATA
                cff5=1.0_r8/Hz(i,j,k)
                t(i,j,k,3,itrc)=cff5*t(i,j,k,nnew,itrc)
#endif
#ifdef DIAGNOSTICS_TS
                DiaTwrk(i,j,k,itrc,iTxdif)=-cff1
                DiaTwrk(i,j,k,itrc,iTydif)=-cff2
                DiaTwrk(i,j,k,itrc,iTsdif)=-cff3
                DiaTwrk(i,j,k,itrc,iThdif)=-cff4
#endif
              END DO
            END DO
          END IF
        END DO K_LOOP2
      END DO T_LOOP
#undef I_RANGE
#undef J_RANGE
      RETURN
      END SUBROUTINE t3dmix4_tile

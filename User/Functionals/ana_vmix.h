      SUBROUTINE ana_vmix (ng, tile, model)
!
!! svn $Id: ana_vmix.h 523 2011-01-05 03:21:38Z arango $
!!======================================================================
!! Copyright (c) 2002-2011 The ROMS/TOMS Group                         !
!!   Licensed under a MIT/X style license                              !
!!   See License_ROMS.txt                                              !
!=======================================================================
!                                                                      !
!  This routine sets vertical mixing coefficients for momentum "Akv"   !
!  and tracers "Akt" (m2/s) using analytical expressions.              !
!                                                                      !
!=======================================================================
!
      USE mod_param
      USE mod_grid
      USE mod_mixing
      USE mod_ncparam
      USE mod_ocean
      USE mod_stepping
!
! Imported variable declarations.
!
      integer, intent(in) :: ng, tile, model

#include "tile.h"
!
      CALL ana_vmix_tile (ng, tile, model,                              &
     &                    LBi, UBi, LBj, UBj,                           &
     &                    IminS, ImaxS, JminS, JmaxS,                   &
     &                    knew(ng),                                     &
     &                    GRID(ng) % h,                                 &
     &                    GRID(ng) % z_r,                               &
     &                    GRID(ng) % z_w,                               &
     &                    OCEAN(ng) % zeta,                             &
     &                    MIXING(ng) % Akv,                             &
     &                    MIXING(ng) % Akt)
!
! Set analytical header file name used.
!
#ifdef DISTRIBUTE
      IF (Lanafile) THEN
#else
      IF (Lanafile.and.(tile.eq.0)) THEN
#endif
        ANANAME(35)=__FILE__
      END IF

      RETURN
      END SUBROUTINE ana_vmix
!
!***********************************************************************
      SUBROUTINE ana_vmix_tile (ng, tile, model,                        &
     &                          LBi, UBi, LBj, UBj,                     &
     &                          IminS, ImaxS, JminS, JmaxS,             &
     &                          knew,                                   &
     &                          h, z_r, z_w, zeta, Akv, Akt)
!***********************************************************************
!
      USE mod_param
      USE mod_scalars
!
#if defined EW_PERIODIC || defined NS_PERIODIC
      USE exchange_3d_mod, ONLY : exchange_w3d_tile
#endif
#ifdef DISTRIBUTE
      USE mp_exchange_mod, ONLY : mp_exchange3d, mp_exchange4d
#endif
!
!  Imported variable declarations.
!
      integer, intent(in) :: ng, tile, model
      integer, intent(in) :: LBi, UBi, LBj, UBj
      integer, intent(in) :: IminS, ImaxS, JminS, JmaxS
      integer, intent(in) :: knew
!
#ifdef ASSUMED_SHAPE
      real(r8), intent(in) :: h(LBi:,LBj:)
      real(r8), intent(in) :: z_r(LBi:,LBj:,:)
      real(r8), intent(in) :: z_w(LBi:,LBj:,0:)
      real(r8), intent(in) :: zeta(LBi:,LBj:,:)
      real(r8), intent(out) :: Akv(LBi:,LBj:,0:)
      real(r8), intent(out) :: Akt(LBi:,LBj:,0:,:)
#else
      real(r8), intent(in) :: h(LBi:UBi,LBj:UBj)
      real(r8), intent(in) :: z_r(LBi:UBi,LBj:UBj,N(ng))
      real(r8), intent(in) :: z_w(LBi:UBi,LBj:UBj,0:N(ng))
      real(r8), intent(in) :: zeta(LBi:UBi,LBj:UBj,3)
      real(r8), intent(out) :: Akv(LBi:UBi,LBj:UBj,0:N(ng))
      real(r8), intent(out) :: Akt(LBi:UBi,LBj:UBj,0:N(ng),NAT)
#endif
!
!  Local variable declarations.
!
#ifdef DISTRIBUTE
# ifdef EW_PERIODIC
      logical :: EWperiodic=.TRUE.
# else
      logical :: EWperiodic=.FALSE.
# endif
# ifdef NS_PERIODIC
      logical :: NSperiodic=.TRUE.
# else
      logical :: NSperiodic=.FALSE.
# endif
#endif
      integer :: i, itrc, j, k

#include "set_bounds.h"
!
!-----------------------------------------------------------------------
!  Set vertical viscosity coefficient (m2/s).
!-----------------------------------------------------------------------
!
#if defined MY_APPLICATION
      DO k=1,N(ng)-1
        DO j=JstrR,JendR
          DO i=IstrR,IendR
            Akv(i,j,k)=???
          END DO
        END DO
      END DO
#else
      ana_vmix.h: no values provided for AKV.
#endif

#if defined EW_PERIODIC || defined NS_PERIODIC
      CALL exchange_w3d_tile (ng, tile,                                 &
     &                        LBi, UBi, LBj, UBj, 0, N(ng),             &
     &                        Akv)
#endif
#ifdef DISTRIBUTE
      CALL mp_exchange3d (ng, tile, model, 1,                           &
     &                    LBi, UBi, LBj, UBj, 0, N(ng),                 &
     &                    NghostPoints, EWperiodic, NSperiodic,         &
     &                    Akv)
#endif
!
!-----------------------------------------------------------------------
!  Set vertical diffusion coefficient (m2/s).
!-----------------------------------------------------------------------
!
#if defined MY_APPLICATION
      DO k=1,N(ng)-1
        DO j=JstrR,JendR
          DO i=IstrR,IendR
            Akt(i,j,k,itemp)=???
          END DO
        END DO
      END DO
#else
      ana_vmix.h: no values provided for AKT.
#endif

#if defined EW_PERIODIC || defined NS_PERIODIC
      DO itrc=1,NAT
        CALL exchange_w3d_tile (ng, tile,                               &
     &                          LBi, UBi, LBj, UBj, 0, N(ng),           &
     &                          Akt(:,:,:,itrc))
      END DO
#endif
#ifdef DISTRIBUTE
      CALL mp_exchange4d (ng, tile, model, 1,                           &
     &                    LBi, UBi, LBj, UBj, 0, N(ng), 1, NAT,         &
     &                    NghostPoints, EWperiodic, NSperiodic,         &
     &                    Akt)
#endif

      RETURN
      END SUBROUTINE ana_vmix_tile

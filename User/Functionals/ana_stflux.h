      SUBROUTINE ana_stflux (ng, tile, model, itrc)
!
!! svn $Id: ana_stflux.h 523 2011-01-05 03:21:38Z arango $
!!======================================================================
!! Copyright (c) 2002-2011 The ROMS/TOMS Group                         !
!!   Licensed under a MIT/X style license                              !
!!   See License_ROMS.txt                                              !
!=======================================================================
!                                                                      !
!  This routine sets kinematic surface flux of tracer type variables   !
!  "stflx" (tracer units m/s) using analytical expressions.            !
!                                                                      !
!=======================================================================
!
      USE mod_param
      USE mod_forces
      USE mod_ncparam
!
! Imported variable declarations.
!
      integer, intent(in) :: ng, tile, model, itrc

#include "tile.h"
!
      CALL ana_stflux_tile (ng, tile, model, itrc,                      &
     &                      LBi, UBi, LBj, UBj,                         &
     &                      IminS, ImaxS, JminS, JmaxS,                 &
#ifdef SHORTWAVE
     &                      FORCES(ng) % srflx,                         &
#endif
#ifdef TL_IOMS
     &                      FORCES(ng) % tl_stflx,                      &
#endif
     &                      FORCES(ng) % stflx)
!
! Set analytical header file name used.
!
#ifdef DISTRIBUTE
      IF (Lanafile) THEN
#else
      IF (Lanafile.and.(tile.eq.0)) THEN
#endif
        ANANAME(31)=__FILE__
      END IF

      RETURN
      END SUBROUTINE ana_stflux
!
!***********************************************************************
      SUBROUTINE ana_stflux_tile (ng, tile, model, itrc,                &
     &                            LBi, UBi, LBj, UBj,                   &
     &                            IminS, ImaxS, JminS, JmaxS,           &
#ifdef SHORTWAVE
     &                            srflx,                                &
#endif
#ifdef TL_IOMS
     &                            tl_stflx,                             &
#endif
     &                            stflx)
!***********************************************************************
!
      USE mod_param
      USE mod_scalars
!
#if defined EW_PERIODIC || defined NS_PERIODIC
      USE exchange_2d_mod, ONLY : exchange_r2d_tile
#endif
#ifdef DISTRIBUTE
      USE mp_exchange_mod, ONLY : mp_exchange2d
#endif
!
!  Imported variable declarations.
!
      integer, intent(in) :: ng, tile, model, itrc
      integer, intent(in) :: LBi, UBi, LBj, UBj
      integer, intent(in) :: IminS, ImaxS, JminS, JmaxS
!
#ifdef ASSUMED_SHAPE
# ifdef SHORTWAVE
      real(r8), intent(in) :: srflx(LBi:,LBj:)
# endif
      real(r8), intent(inout) :: stflx(LBi:,LBj:,:)
# ifdef TL_IOMS
      real(r8), intent(inout) :: tl_stflx(LBi:,LBj:,:)
# endif
#else
# ifdef SHORTWAVE
      real(r8), intent(in) :: srflx(LBi:UBi,LBj:UBj)
# endif
      real(r8), intent(inout) :: stflx(LBi:UBi,LBj:UBj,NT(ng))
# ifdef TL_IOMS
      real(r8), intent(inout) :: tl_stflx(LBi:UBi,LBj:UBj,NT(ng))
# endif
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
      integer :: i, j

#include "set_bounds.h"
!
!-----------------------------------------------------------------------
!  Set kinematic surface heat flux (degC m/s) at horizontal
!  RHO-points.
!-----------------------------------------------------------------------
!
      IF (itrc.eq.itemp) THEN
#if defined MY_APPLICATION
        DO j=JstrR,JendR
          DO i=IstrR,IendR
            stflx(i,j,itrc)=???
          END DO
        END DO
#else
        DO j=JstrR,JendR
          DO i=IstrR,IendR
            stflx(i,j,itrc)=0.0_r8
          END DO
        END DO
#endif
!
!-----------------------------------------------------------------------
!  Set kinematic surface freshwater flux (m/s) at horizontal
!  RHO-points, scaling by surface salinity is done in STEP3D.
!-----------------------------------------------------------------------
!
      ELSE IF (itrc.eq.isalt) THEN
#if defined MY_APPLICATION
        DO j=JstrR,JendR
          DO i=IstrR,IendR
            stflx(i,j,itrc)=???
          END DO
        END DO
#else
        DO j=JstrR,JendR
          DO i=IstrR,IendR
            stflx(i,j,itrc)=0.0_r8
          END DO
        END DO
#endif
!
!-----------------------------------------------------------------------
!  Set kinematic surface flux (T m/s) of passive tracers, if any.
!-----------------------------------------------------------------------
!
      ELSE
#if defined MY_APPLICATION
        DO j=JstrR,JendR
          DO i=IstrR,IendR
            stflx(i,j,itrc)=???
          END DO
        END DO
      END IF
#else
        DO j=JstrR,JendR
          DO i=IstrR,IendR
            stflx(i,j,itrc)=0.0_r8
          END DO
        END DO
      END IF
#endif

#if defined EW_PERIODIC || defined NS_PERIODIC
      CALL exchange_r2d_tile (ng, tile,                                 &
     &                        LBi, UBi, LBj, UBj,                       &
     &                        stflx(:,:,itrc))
# ifdef TL_IOMS
      CALL exchange_r2d_tile (ng, tile,                                 &
     &                        LBi, UBi, LBj, UBj,                       &
     &                        tl_stflx(:,:,itrc))
# endif
#endif
#ifdef DISTRIBUTE
      CALL mp_exchange2d (ng, tile, model, 1,                           &
     &                    LBi, UBi, LBj, UBj,                           &
     &                    NghostPoints, EWperiodic, NSperiodic,         &
     &                    stflx(:,:,itrc))
# ifdef TL_IOMS
      CALL mp_exchange2d (ng, tile, model, 1,                           &
     &                    LBi, UBi, LBj, UBj,                           &
     &                    NghostPoints, EWperiodic, NSperiodic,         &
     &                    tl_stflx(:,:,itrc))
# endif
#endif

      RETURN
      END SUBROUTINE ana_stflux_tile

      MODULE ocean_control_mod
!
!svn $Id: tlcheck_ocean.h 553 2011-04-22 21:30:04Z arango $
!================================================== Hernan G. Arango ===
!  Copyright (c) 2002-2011 The ROMS/TOMS Group       Andrew M. Moore   !
!    Licensed under a MIT/X style license                              !
!    See License_ROMS.txt                                              !
!=======================================================================
!                                                                      !
!  ROMS/TOMS Tangent Linear Model Linearization Test:                  !
!                                                                      !
!  This driver is used to check the "linearization" of the tangent     !
!  linear model using a structure similar to the gradient test.        !
!                                                                      !
!  The subroutines in this driver control the initialization, time-    !
!  stepping,  and  finalization of ROMS/TOMS  model following ESMF     !
!  conventions:                                                        !
!                                                                      !
!     ROMS_initialize                                                  !
!     ROMS_run                                                         !
!     ROMS_finalize                                                    !
!                                                                      !
!=======================================================================
!
      implicit none

      PRIVATE
      PUBLIC  :: ROMS_initialize
      PUBLIC  :: ROMS_run
      PUBLIC  :: ROMS_finalize

      CONTAINS

      SUBROUTINE ROMS_initialize (first, mpiCOMM)
!
!=======================================================================
!                                                                      !
!  This routine allocates and initializes ROMS/TOMS state variables    !
!  and internal and external parameters.                               !
!                                                                      !
!=======================================================================
!
      USE mod_param
      USE mod_parallel
      USE mod_fourdvar
      USE mod_iounits
      USE mod_scalars

#ifdef MCT_LIB
!
# ifdef AIR_OCEAN
      USE ocean_coupler_mod, ONLY : initialize_ocn2atm_coupling
# endif
# ifdef WAVES_OCEAN
      USE ocean_coupler_mod, ONLY : initialize_ocn2wav_coupling
# endif
#endif
!
!  Imported variable declarations.
!
      logical, intent(inout) :: first

      integer, intent(in), optional :: mpiCOMM
!
!  Local variable declarations.
!
      logical :: allocate_vars = .TRUE.

#ifdef DISTRIBUTE
      integer :: MyError, MySize
#endif
      integer :: ng, thread

#ifdef DISTRIBUTE
!
!-----------------------------------------------------------------------
!  Set distribute-memory (MPI) world communictor.
!-----------------------------------------------------------------------
!
      IF (PRESENT(mpiCOMM)) THEN
        OCN_COMM_WORLD=mpiCOMM
      ELSE
        OCN_COMM_WORLD=MPI_COMM_WORLD
      END IF
      CALL mpi_comm_rank (OCN_COMM_WORLD, MyRank, MyError)
      CALL mpi_comm_size (OCN_COMM_WORLD, MySize, MyError)
#endif
!
!-----------------------------------------------------------------------
!  On first pass, initialize model parameters a variables for all
!  nested/composed grids.  Notice that the logical switch "first"
!  is used to allow multiple calls to this routine during ensemble
!  configurations.
!-----------------------------------------------------------------------
!
      IF (first) THEN
        first=.FALSE.
!
!  Initialize parallel control switches. These scalars switches are
!  independent from standard input parameters.
!
        CALL initialize_parallel
!
!  Read in model tunable parameters from standard input. Allocate and
!  initialize variables in several modules after the number of nested
!  grids and dimension parameters are known.
!
        CALL inp_par (iNLM)
        IF (exit_flag.ne.NoError) RETURN
!
!  Initialize internal wall clocks. Notice that the timings does not
!  includes processing standard input because several parameters are
!  needed to allocate clock variables.
!
        IF (Master) THEN
          WRITE (stdout,10)
 10       FORMAT (/,' Process Information:',/)
        END IF
!
        DO ng=1,Ngrids
!$OMP PARALLEL DO PRIVATE(thread) SHARED(numthreads)
          DO thread=0,numthreads-1
            CALL wclock_on (ng, iNLM, 0)
          END DO
!$OMP END PARALLEL DO
        END DO
!
!  Allocate and initialize modules variables.
!
        CALL mod_arrays (allocate_vars)
!
!  Allocate and initialize observation arrays.
!
        CALL initialize_fourdvar

      END IF

#if defined MCT_LIB && (defined AIR_OCEAN || defined WAVES_OCEAN)
!
!-----------------------------------------------------------------------
!  Initialize coupling streams between model(s).
!-----------------------------------------------------------------------
!
      DO ng=1,Ngrids
# ifdef AIR_OCEAN
        CALL initialize_ocn2atm_coupling (ng, MyRank)
# endif
# ifdef WAVES_OCEAN
        CALL initialize_ocn2wav_coupling (ng, MyRank)
# endif
      END DO
#endif

      RETURN
      END SUBROUTINE ROMS_initialize

      SUBROUTINE ROMS_run (RunInterval)
!
!=======================================================================
!                                                                      !
!  This routine time-steps ROMS/TOMS nonlinear, tangent linear and     !
!  adjoint models.                                                     !
!                                                                      !
!=======================================================================
!
      USE mod_param
      USE mod_parallel
      USE mod_fourdvar
      USE mod_iounits
      USE mod_ncparam
      USE mod_netcdf
      USE mod_scalars
      USE mod_stepping
!
      USE dotproduct_mod, ONLY : ad_dotproduct
!
!  Imported variable declarations
!
      real(r8), intent(in) :: RunInterval            ! seconds
!
!  Local variable declarations.
!
      integer :: IniRec, i, ng, status
      integer :: subs, tile, thread

      real(r8) :: gp, hp, p
!
!-----------------------------------------------------------------------
!  Run tangent linear model test.
!-----------------------------------------------------------------------
!
!  Currently, the tangent linear model test cannot be run over
!  nested grids.
!
      IF (Ngrids.gt.1) THEN
        WRITE (stdout,10) 'Nested grids are not allowed, Ngrids = ',    &
                          Ngrids
        STOP
      END IF
!
!  Initialize relevant parameters.
!
      DO ng=1,Ngrids
        Lold(ng)=1
        Lnew(ng)=1
        nTLM(ng)=nHIS(ng)                      ! to allow IO comparison
      END DO
      Nrun=1
      Ipass=1
      ERstr=1
      ERend=MAXVAL(NstateVar)+1
      IniRec=1
      ig1count=0
      ig2count=0
      LcycleTLM=.FALSE.
!
!  Initialize nonlinear model with first guess initial conditions.
!
      DO ng=1,Ngrids
        CALL initial (ng)
        IF (exit_flag.ne.NoError) RETURN
      END DO
!
!  Run nonlinear model. Extract and store nonlinear model values at
!  observation locations.
!
      DO ng=1,Ngrids
        IF (Master) THEN
          WRITE (stdout,20) 'NL', ng, ntstart(ng), ntend(ng)
        END IF
        wrtNLmod(ng)=.TRUE.
        wrtTLmod(ng)=.FALSE.
      END DO

#ifdef SOLVE3D
      CALL main3d (RunInterval)
#else
      CALL main2d (RunInterval)
#endif
      IF (exit_flag.ne.NoError) RETURN
!
!  Close current nonlinear model history file.
!
      SourceFile='tlcheck_ocean.h, ROMS_run'
      DO ng=1,Ngrids
        CALL netcdf_close (ng, iNLM, HIS(ng)%ncid)
        IF (exit_flag.ne.NoError) RETURN
        wrtNLmod(ng)=.FALSE.
        wrtTLmod(ng)=.TRUE.
      END DO
!
!  Save and Report cost function between nonlinear model and
!  observations.
!
      DO ng=1,Ngrids
        DO i=0,NstateVar(ng)
          FOURDVAR(ng)%CostFunOld(i)=FOURDVAR(ng)%CostFun(i)
        END DO
        IF (Master) THEN
          WRITE (stdout,40) FOURDVAR(ng)%CostFunOld(0)
          DO i=1,NstateVar(ng)
            IF (FOURDVAR(ng)%CostFunOld(i).gt.0.0_r8) THEN
              IF (i.eq.1) THEN
                WRITE (stdout,50) FOURDVAR(ng)%CostFunOld(i),           &
     &                            TRIM(Vname(1,idSvar(i)))
              ELSE
                WRITE (stdout,60) FOURDVAR(ng)%CostFunOld(i),           &
     &                            TRIM(Vname(1,idSvar(i)))
              END IF
            END IF
          END DO
        END IF
      END DO
!
!  Initialize the adjoint model from rest.
!
      DO ng=1,Ngrids
        CALL ad_initial (ng, .TRUE.)
        IF (exit_flag.ne.NoError) RETURN
      END DO
!
!  Time-step adjoint model: Compute model state gradient, GRAD(J).
!  Force the adjoint model with the adjoint misfit between nonlinear
!  model and observations.
!
      DO ng=1,Ngrids
        IF (Master) THEN
          WRITE (stdout,20) 'AD', ng, ntstart(ng), ntend(ng)
        END IF
      END DO

#ifdef SOLVE3D
      CALL ad_main3d (RunInterval)
#else
      CALL ad_main2d (RunInterval)
#endif
      IF (exit_flag.ne.NoError) RETURN
!
!-----------------------------------------------------------------------
!  Perturb each tangent linear state variable using the steepest decent
!  direction by grad(J). If ndefTLM < 0, suppress IO for both nonlinear
!  and tangent linear models in the outer and inner loops.
!-----------------------------------------------------------------------
!
!  Load adjoint solution.
!
      DO ng=1,Ngrids
        CALL get_state (ng, iADM, 3, ADM(ng)%name, ADM(ng)%Rindex,      &
     &                  Lnew(ng))
        IF (exit_flag.ne.NoError) RETURN
      END DO
!
!  Compute adjoint solution dot product for scaling purposes.
!
      DO ng=1,Ngrids
!$OMP PARALLEL DO PRIVATE(thread,subs,tile) SHARED(numthreads)
        DO thread=0,numthreads-1
          subs=NtileX(ng)*NtileE(ng)/numthreads
          DO tile=subs*thread,subs*(thread+1)-1
            CALL ad_dotproduct (ng, TILE, Lnew(ng))
          END DO
        END DO
!$OMP END PARALLEL DO
      END DO

#ifdef SOLVE3D
!
!  OUTER LOOP: First, Perturb all state variables (zeta, u, v, t) at
!  ==========  once. Then, perturb one state variable at the time.
!              Notice, that ubar and vbar are not perturbed. They are
!              computed by vertically integarting u and v.
!
#else
!
!  OUTER LOOP: First, perturb all state variables (zeta, ubar, vbar) at
!  ==========  once. Then, perturb one state variable at the time.
!
#endif
      OUTER_LOOP : DO outer=ERstr,ERend

        DO ng=1,Ngrids
          CALL get_state (ng, iNLM, 1, FWD(ng)%name, IniRec, Lnew(ng))
          IF (exit_flag.ne.NoError) RETURN
        END DO
!
!  INNER LOOP: scale perturbation amplitude by selecting "p" scalar,
!  ==========  such that:
!                              p = 10 ** FLOAT(-inner)
!
        INNER_LOOP : DO inner=1,Ninner
!
!  Add a perturbation to the nonlinear state initial conditions
!  according with the outer and inner loop iterations. The added
!  term is a function of the steepest descent direction defined
!  by grad(J) times the perturbation amplitude "p".
!
          DO ng=1,Ngrids
            CALL initial (ng, .FALSE.)
            IF (exit_flag.ne.NoError) RETURN

            WRITE (HIS(ng)%name,70) TRIM(HIS(ng)%base), Nrun

            IF (ndefTLM(ng).lt.0) THEN
              LdefHIS(ng)=.FALSE.              ! suppress IO
              LwrtHIS(ng)=.FALSE.
            ELSE
              LdefHIS(ng)=.TRUE.
            END IF
            wrtNLmod(ng)=.TRUE.
            wrtTLmod(ng)=.FALSE.
          END DO
!
!  Time-step nonlinear model: compute perturbed nonlinear state.
!
          DO ng=1,Ngrids
            IF (Master) THEN
              WRITE (stdout,20) 'NL', ng, ntstart(ng), ntend(ng)
            END IF
          END DO

#ifdef SOLVE3D
          CALL main3d (RunInterval)
#else
          CALL main2d (RunInterval)
#endif
          IF (exit_flag.ne.NoError) RETURN
!
!  Get current nonlinear model trajectory.
!
          DO ng=1,Ngrids
            FWD(ng)%name=TRIM(HIS(ng)%base)//'.nc'
            CALL get_state (ng, iNLM, 1, FWD(ng)%name, IniRec, Lnew(ng))
            IF (exit_flag.ne.NoError) RETURN
          END DO
!
!  Initialize tangent linear with the steepest decent direction
!  (adjoint state, GRAD(J)) times the perturbation amplitude "p".
!
          DO ng=1,Ngrids
            CALL tl_initial (ng, .FALSE.)
            IF (exit_flag.ne.NoError) RETURN

            WRITE (TLM(ng)%name,70) TRIM(TLM(ng)%base), Nrun

            IF (ndefTLM(ng).lt.0) THEN
              LdefTLM(ng)=.FALSE.              ! suppress IO
              LwrtTLM(ng)=.FALSE.
            ELSE
              LdefTLM(ng)=.TRUE.
            END IF
          END DO
!
!  Time-step tangent linear model:  Compute misfit cost function
!  between model (nonlinear + tangent linear) and observations.
!
          DO ng=1,Ngrids
            IF (Master) THEN
              WRITE (stdout,20) 'TL', ng, ntstart(ng), ntend(ng)
            END IF
          END DO

#ifdef SOLVE3D
          CALL tl_main3d (RunInterval)
#else
          CALL tl_main2d (RunInterval)
#endif
          IF (exit_flag.ne.NoError) RETURN
!
!  Close current tangent linear model history file.
!
          SourceFile='tlcheck_ocean.h, ROMS_run'
          DO ng=1,Ngrids
            CALL netcdf_close (ng, iTLM, TLM(ng)%ncid)
            IF (exit_flag.ne.NoError) RETURN
          END DO
!
!  Advance model run counter.
!
          Nrun=Nrun+1

        END DO INNER_LOOP

      END DO OUTER_LOOP
!
!  Report dot products.
!
      DO ng=1,Ngrids
        IF (Master) THEN
          WRITE (stdout,80)                                             &
     &      'TLM Test - Dot Products Summary: p, g1, g2, (g1-g2)/g1'
          inner=1
          DO i=1,MIN(ig1count,ig2count)
            p=10.0_r8**FLOAT(-inner)
            IF (MOD(i,1+ntimes(ng)/nTLM(ng)).eq.0) inner=inner+1
            WRITE (stdout,90) i, p, g1(i), g2(i), (g1(i)-g2(i))/g1(i)
            IF ((MOD(i,1+ntimes(ng)/nTLM(ng)).eq.0).and.                &
     &          (MOD(i,Ninner*(1+ntimes(ng)/nTLM(ng))).ne.0)) THEN
              WRITE (stdout,100)
            ELSE IF (MOD(i,Ninner*(1+ntimes(ng)/nTLM(ng))).eq.0) THEN
              inner=1
              WRITE (stdout,110)
            END IF
          END DO
        END IF
      END DO
!
 10   FORMAT (/,a,i3,/)
 20   FORMAT (/,1x,a,1x,'ROMS/TOMS : started time-stepping:',           &
     &        ' (Grid: ',i2.2,' TimeSteps: ',i8.8,' - ',i8.8,')',/)
 40   FORMAT (/,' Nonlinear Model Cost Function = ',1p,e21.14)
 50   FORMAT (' --------------- ','cost function = ',1p,e21.14,2x,a)
 60   FORMAT (17x,'cost function = ',1p,e21.14,2x,a)
 70   FORMAT (a,'_',i3.3,'.nc')
 80   FORMAT (/,a,/)
 90   FORMAT (i4,2x,1pe8.1,3(1x,1p,e20.12,0p))
100   FORMAT (77('.'))
110   FORMAT (77('-'))

      RETURN
      END SUBROUTINE ROMS_run

      SUBROUTINE ROMS_finalize
!
!=======================================================================
!                                                                      !
!  This routine terminates ROMS/TOMS nonlinear, tangent linear, and    !
!  adjoint models execution.                                           !
!                                                                      !
!=======================================================================
!
      USE mod_param
      USE mod_parallel
      USE mod_iounits
      USE mod_ncparam
      USE mod_scalars
!
!  Local variable declarations.
!
      integer :: Fcount, ng, thread
!
!-----------------------------------------------------------------------
!  If blowing-up, save latest model state into RESTART NetCDF file.
!-----------------------------------------------------------------------
!
!  If cycling restart records, write solution into the next record.
!
      IF (exit_flag.eq.1) THEN
        DO ng=1,Ngrids
          IF (LwrtRST(ng)) THEN
            IF (Master) WRITE (stdout,10)
 10         FORMAT (/,' Blowing-up: Saving latest model state into ',   &
     &                ' RESTART file',/)
            Fcount=RST(ng)%Fcount
            IF (LcycleRST(ng).and.(RST(ng)%Nrec(Fcount).ge.2)) THEN
              RST(ng)%Rindex=2
              LcycleRST(ng)=.FALSE.
            END IF
            blowup=exit_flag
            exit_flag=NoError
            CALL wrt_rst (ng)
          END IF
        END DO
      END IF
!
!-----------------------------------------------------------------------
!  Stop model and time profiling clocks.  Close output NetCDF files.
!-----------------------------------------------------------------------
!
!  Stop time clocks.
!
      IF (Master) THEN
        WRITE (stdout,20)
 20     FORMAT (/,' Elapsed CPU time (seconds):',/)
      END IF

      DO ng=1,Ngrids
!$OMP PARALLEL DO PRIVATE(thread) SHARED(numthreads)
        DO thread=0,numthreads-1
          CALL wclock_off (ng, iNLM, 0)
        END DO
!$OMP END PARALLEL DO
      END DO
!
!  Close IO files.
!
      CALL close_out

      RETURN
      END SUBROUTINE ROMS_finalize

      END MODULE ocean_control_mod

@set paramFile=parameters-metropolis.txt
@set threadCount=3

@rem @set model=UP_DOWN_MODEL
@set model=CONTINUOUS_SPIN_MODEL

@rem @set mode=REINITIALIZE
@set mode=RANDOMIZE

@set out_head=metropolis


@rem -- Find the next open file name
@set prgm=metropolis
@set id=0
:INC_ID
@set /a id=%id%+1
@if %id% LEQ 0 goto STOP
@set out_file="out\%out_head%, %date:~4,2%-%date:~7,2%-%date:~10,4%, %id%.xml"
@if exist %out_file% goto INC_ID

@date /t
@time /t
@echo ----------------------------------------
bin\%prgm% %paramFile% %out_file% %model% %mode% %threadCount%
@echo ----------------------------------------
@date /t
@time /t
@goto DONE

:STOP
@echo Error (%out_file%): No more possible file names exist!

:DONE
@pause

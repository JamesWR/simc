call python casc_extract.py -m batch --cdn --ptr -o wow
cd wow
dir /b /a:D /O:-D>hi.txt
set /p wowdir=<hi.txt
set /p oldname=<hi.txt
set wowdir=%wowdir:~4,-14%
ren %oldname% %wowdir%
del hi.txt
cd ..
cd ..
set curr=%cd%
cd dbc_extract
call generate.bat ptr %wowdir% %curr%\casc_extract\wow
pause
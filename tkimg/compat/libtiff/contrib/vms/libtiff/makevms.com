$!========================================================================
$!
$! Name      : MAKEVMS
$!
$! Purpose   : Compile TIFF library
$!
$! Arguments : If P1 is DEBUG, compile with debug
$!
$! Created   1-DEC-1994   Karsten Spang
$!
$!========================================================================
$   CURRENT_DIR=F$ENVIRONMENT("DEFAULT")
$   ON CONTROL_Y THEN GOTO EXIT
$   ON ERROR THEN GOTO EXIT
$!
$! Get hold on definitions
$!
$!  Older versions of VMS may not recoqnize the "ARCH_NAME" keyword
$!  This happens only on VAX
$!
$   SAVE_MESS=F$ENVIRONMENT("MESSAGE")
$   SET MESSAGE/NOID/NOFAC/NOSEV/NOTEXT
$   ARCH=F$GETSYI("ARCH_NAME")
$   SET MESSAGE 'SAVE_MESS'
$   IF F$TYPE(ARCH).EQS."" THEN ARCH="VAX"
$   ARCH=F$EDIT(ARCH,"UPCASE")
$!
$   DEFINE/NOLOG SYS SYS$LIBRARY
$   THIS_FILE=F$ENVIRONMENT("PROCEDURE")
$   PROC_NAME=F$PARSE(THIS_FILE,,,"NAME","SYNTAX_ONLY")
$   THIS_DIR=F$PARSE(THIS_FILE,,,"DEVICE","SYNTAX_ONLY")+ -
        F$PARSE(THIS_FILE,,,"DIRECTORY","SYNTAX_ONLY")
$   SET DEFAULT 'THIS_DIR'
$   IF ARCH.EQS."ALPHA"
$   THEN
$       CONF_FP="HAVE_IEEEFP=1"
$   ELSE
$       CONF_FP="HAVE_IEEEFP=0"
$   ENDIF
$   CONF_LIBRARY="BSDTYPES,HAVE_MMAP"
$   IF P1.EQS."DEBUG"
$   THEN
$       DEBUG_OPTIONS="/DEBUG/NOOPTIMIZE"
$       CONF_LIBRARY=CONF_LIBRARY+",DEBUG"
$       LINK_OPTIONS="/DEBUG"
$   ELSE
$       DEBUG_OPTIONS=""
$       LINK_OPTIONS=""
$   ENDIF
$   DEFINES="/DEFINE=("+CONF_FP+","+CONF_LIBRARY+")"
$   C_COMPILE="CC"+DEBUG_OPTIONS+DEFINES
$   IF ARCH.EQS."ALPHA"
$   THEN
$!
$!      You may want a different floating point option
$!
$       C_COMPILE=C_COMPILE+ -
            "/FLOAT=IEEE_FLOAT/PREFIX_LIBRARY_ENTRIES=ALL_ENTRIES"
$   ENDIF
$!
$   SOURCES="TIF_AUX,TIF_CLOSE,TIF_CODEC,TIF_COMPRESS,"+ -
        "TIF_DIR,TIF_DIRINFO,TIF_DIRREAD,TIF_DIRWRITE,"+ -
        "TIF_DUMPMODE,TIF_ERROR,TIF_FAX3,TIF_FAX3SM,TIF_FLUSH,TIF_GETIMAGE,"+ -
-!        "TIF_JPEG,"+ -
        "TIF_LZW,TIF_NEXT,TIF_OPEN,TIF_PACKBITS,TIF_PIXARLOG,TIF_PREDICT,"+ -
        "TIF_PRINT,TIF_READ,TIF_STRIP,TIF_SWAB,TIF_THUNDER,TIF_TILE,"+ -
        "TIF_VERSION,TIF_VMS,TIF_WARNING,TIF_WRITE"
! ",TIF_ZIP"
$   LIBFILE="TIFF"
$   IF F$SEARCH(LIBFILE+".OLB").EQS."" THEN -
        LIBRARY/CREATE 'LIBFILE'
$!
$! Create the port library
$!
$   LIBPORT="[-.PORT]PORT"
$   IF F$SEARCH(LIBPORT+".OLB") .EQS "" 
$   THEN
$       WRITE SYS$OUTPUT "Creating PORT.OLB"
$       LIBRARY/CREATE 'LIBPORT'
$       CREATE DUM.C
main(){getopt();strtoul();strcasecmp();}
$       C_COMPILE DUM
$       SET MESSAGE/ID/FAC/SEV/TEXT
$       DEFINE/USER SYS$OUTPUT LINK.WARNINGS
$       DEFINE/USER SYS$ERROR NLA0:
$       IF ARCH.EQS."ALPHA"
$       THEN
$           LINK DUM
$       ELSE
$           LINK DUM,SYS$INPUT:/OPTIONS
SYS$SHARE:VAXCRTL/SHARE
$       ENDIF
$       DELETE DUM.C;,DUM.OBJ;,DUM.EXE;
$       SEARCH/OUT=MISSING.OBJECTS LINK.WARNINGS LINK-I-UDFSYM
$       DELETE LINK.WARNINGS;
$       OPEN/READ MISSING MISSING.OBJECTS
$NEXTMIS:
$           READ/END=NOMOREMIS MISSING LINE
$           LINE=F$EDIT(LINE,"TRIM,COMPRESS,UPCASE")
$           LINE=F$ELEMENT(1," ",LINE)
$           IF LINE .EQS. " " THEN GOTO NEXTMIS
$           WRITE SYS$OUTPUT "   "+LINE
$           C_COMPILE/OBJECT=[-.PORT]'LINE' [-.PORT]'LINE'
$           LIBRARY 'LIBPORT' [-.PORT]'LINE'
$           DELETE [-.PORT]'LINE'.OBJ;
$       GOTO NEXTMIS
$NOMOREMIS:
$       CLOSE MISSING
$       DELETE MISSING.OBJECTS;
$   ENDIF
$!
$! Create VERSION.H
$!
$   IF F$SEARCH("VERSION.H").EQS.""
$   THEN
$       WRITE SYS$OUTPUT "Creating VERSION.H"
$       IF F$SEARCH("MKVERSION.EXE").EQS.""
$       THEN
$           IF F$SEARCH("MKVERSION.OBJ").EQS.""
$           THEN
$               C_COMPILE MKVERSION
$           ENDIF
$           IF ARCH.EQS."ALPHA"
$           THEN
$               LINK MKVERSION,'LIBPORT'/LIBRARY
$           ELSE
$               LINK MKVERSION,'LIBPORT'/LIBRARY,SYS$INPUT:/OPTIONS
SYS$SHARE:VAXCRTL/SHARE
$           ENDIF
$           DELETE MKVERSION.OBJ;*
$       ENDIF
$       MKVERSION:=$'THIS_DIR'MKVERSION
$       MKVERSION -V [-]VERSION -A [-.DIST]TIFF.ALPHA VERSION.H
$       DELETE MKVERSION.EXE;*
$   ENDIF
$!
$! Create TIF_FAX3SM.C
$!
$   IF F$SEARCH("TIF_FAX3SM.C").EQS.""
$   THEN
$       WRITE SYS$OUTPUT "Creating FAX3SM.C"
$       IF F$SEARCH("MKG3STATES.EXE").EQS.""
$       THEN
$           IF F$SEARCH("MKG3STATES.OBJ").EQS.""
$           THEN
$               C_COMPILE MKG3STATES
$           ENDIF
$           IF ARCH.EQS."ALPHA"
$           THEN
$               LINK MKG3STATES,'LIBPORT'/LIBRARY
$           ELSE
$               LINK MKG3STATES,'LIBPORT'/LIBRARY,SYS$INPUT:/OPTIONS
SYS$SHARE:VAXCRTL/SHARE
$           ENDIF
$           DELETE MKG3STATES.OBJ;*
$       ENDIF
$       MKG3STATES:=$'THIS_DIR'MKG3STATES
$       MKG3STATES -c const TIF_FAX3SM.C
$       DELETE MKG3STATES.EXE;*
$   ENDIF
$!
$! Loop over modules
$!
$   NUMBER=0
$COMPILE_LOOP:
$       FILE=F$ELEMENT(NUMBER,",",SOURCES)
$       IF FILE.EQS."," THEN GOTO END_COMPILE
$       C_FILE=F$PARSE(FILE,".C",,,"SYNTAX_ONLY")
$       C_FILE=F$SEARCH(C_FILE)
$       IF C_FILE.EQS.""
$       THEN
$           WRITE SYS$OUTPUT "Source file "+FILE+" not found"
$           GOTO EXIT
$       ENDIF
$       C_DATE=F$CVTIME(F$FILE_ATTRIBUTES(C_FILE,"RDT"))
$       OBJ_FILE=F$PARSE("",".OBJ",C_FILE,,"SYNTAX_ONLY")
$       OBJ_FILE=F$EXTRACT(0,F$LOCATE(";",OBJ_FILE),OBJ_FILE)
$       FOUND_OBJ_FILE=F$SEARCH(OBJ_FILE)
$       IF FOUND_OBJ_FILE.EQS.""
$       THEN
$           OBJ_DATE=""
$       ELSE
$           OBJ_DATE=F$CVTIME(F$FILE_ATTRIBUTES(FOUND_OBJ_FILE,"CDT"))
$       ENDIF
$       IF OBJ_DATE.LTS.C_DATE
$       THEN
$           WRITE SYS$OUTPUT "Compiling "+FILE
$           ON ERROR THEN CONTINUE
$           C_COMPILE/OBJECT='OBJ_FILE' 'C_FILE'
$           ON ERROR THEN GOTO EXIT
$           LIBRARY 'LIBFILE' 'OBJ_FILE'
$
$       ENDIF
$       NUMBER=NUMBER+1
$   GOTO COMPILE_LOOP
$END_COMPILE:
$   IF ARCH.EQS."ALPHA"
$   THEN
$       OPT_FILE="TIFFSHRAXP"
$   ELSE
$       OPT_FILE="TIFFSHRVAX"
$       FILE="TIFFVEC"
$       MAR_FILE=F$PARSE(FILE,".MAR",,,"SYNTAX_ONLY")
$       MAR_FILE=F$SEARCH(MAR_FILE)
$       MAR_FILE=F$SEARCH("TIFFVEC.MAR")
$       MAR_DATE=F$CVTIME(F$FILE_ATTRIBUTES(MAR_FILE,"RDT"))
$       OBJ_FILE=F$PARSE("",".OBJ",MAR_FILE,,"SYNTAX_ONLY")
$       OBJ_FILE=F$EXTRACT(0,F$LOCATE(";",OBJ_FILE),OBJ_FILE)
$       FOUND_OBJ_FILE=F$SEARCH(OBJ_FILE)
$       IF FOUND_OBJ_FILE.EQS.""
$       THEN
$           OBJ_DATE=""
$       ELSE                                 
$           OBJ_DATE=F$CVTIME(F$FILE_ATTRIBUTES(FOUND_OBJ_FILE,"CDT"))
$       ENDIF
$       IF OBJ_DATE.LTS.MAR_DATE
$       THEN
$           WRITE SYS$OUTPUT "Compiling "+FILE
$           MACRO 'MAR_FILE'
$           LIBRARY 'LIBFILE' 'OBJ_FILE'
$           PURGE 'OBJ_FILE'
$       ENDIF
$   ENDIF
$   WRITE SYS$OUTPUT "Creating shareable library"
$   LINK/SHAREABLE='THIS_DIR'TIFFSHR'LINK_OPTIONS' 'OPT_FILE'/OPTIONS
$   PURGE/LOG TIFFSHR.EXE
$EXIT:
$   SET DEFAULT 'CURRENT_DIR'
$   EXIT

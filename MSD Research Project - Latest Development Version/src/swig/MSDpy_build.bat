@rem vc
swig -python "%CD%/MSD.i" 
cl /EHsc /DL /I C:\Users\mathh\AppData\Local\Programs\Python\Python310/include src/swig/MSD_wrap.c
# Molecular-Spintronics-Research-Project

Author: Christopher D'Angelo
	with much help and guidance from Prof. Pawan Tyagi

Version 5.3.1 is the current version as of 12-15-2020

v6.0.0 is a purposed change to the model to switch from an array structure to a linked structure for more flexibility, as well as adding an enhanched UI/GUI.
Currently these changes are not being worked on.

v5.*.* is the current development version. Planned additions:
	1. (Done.) Create support for input file for iterate.exe that allows for control of exact initial spin values for particular atoms.
	2. Create auxiluary program to compute auto-corralation between FM and mol., heat capacity of a sub-region of the model, and magnetic suseptibility of specicif regions of model.
	3. (Done?) Add toggle to change mol. so it can go from having 4 edges (square shaped), to only 2 edges (paralell lines shaped).
	4. Add skyrmion effect by adding new energy term
	5. Add FM layers
	See Taskade for full list of planned additions: https://www.taskade.com/v/p1KbJ8wL5XbigKjX

(11-1-2020) Version 5.1.0 is now complete and ready to be beta tested.
(11-3-2020) Version 5.1.1 has a major bug fix in calculations!
(11-17-2020) Version 5.2.0 added the ability to set the seed in MSD. (Only added this functionallity to the iterate.cpp program so far.) 
(12-15-2020) Version 5.3.0 change how the spin flux vectors are changed durring metropolis. Now their magnitude is randomized as well.
(12-15-2020) Version 5.3.1 allow magnetize programs to align the magnetic field in any direction (not just the y direction) using spherical coord.

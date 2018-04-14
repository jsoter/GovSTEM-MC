/***********************************************************
 *  Copyright Univ. of Texas M.D. Anderson Cancer Center
 *  1992.
 *
 *	main program for Monte Carlo simulation of photon
 *	distribution in multi-layered turbid media.
 *
 ****/

/****
 *	THINKCPROFILER is defined to generate profiler calls in 
 *	Think C. If 1, remember to turn on "Generate profiler 
 *	calls" in the options menu. 
 ****/
#define THINKCPROFILER 0	

/* GNU cc does not support difftime() and CLOCKS_PER_SEC.*/
#define GNUCC 0

#if THINKCPROFILER
#include <profile.h>
#include <console.h>

#endif

#include "mcml.h"


/*	Declare before they are used in main(). */
FILE *GetFile(char *);
short ReadNumRuns(FILE* );
void ReadParm(FILE* , InputStruct * );
void CheckParm(FILE* , InputStruct * );
void InitOutputData(InputStruct, OutStruct *);
void FreeData(InputStruct, OutStruct *);
double Rspecular(LayerStruct * );
void LaunchPhoton(double, LayerStruct *, PhotonStruct *);
void HopDropSpin(InputStruct  *  In_Ptr, PhotonStruct *  Photon_Ptr, OutStruct    *  Out_Ptr, MultiScatterStruct multi);
void SumScaleResult(InputStruct, OutStruct *);
void WriteResult(InputStruct, OutStruct, char *);



/***********************************************************
 *	If F = 0, reset the clock and return 0.
 *
 *	If F = 1, pass the user time to Msg and print Msg on 
 *	screen, return the real time since F=0. 
 *
 *	If F = 2, same as F=1 except no printing.  
 *
 *	Note that clock() and time() return user time and real 
 *	time respectively.
 *	User time is whatever the system allocates to the 
 *	running of the program; 
 *	real time is wall-clock time.  In a time-shared system,
 *	they need not be the same.
 *	
 *	clock() only hold 16 bit integer, which is about 32768 
 *	clock ticks.
 ****/
time_t PunchTime(char F, char *Msg)
{
#if GNUCC
  return(0);
#else
  static clock_t ut0;	/* user time reference. */
  static time_t  rt0;	/* real time reference. */
  double secs;
  char s[STRLEN];
  
  if(F==0) {
    ut0 = clock();
    rt0 = time(NULL);
    return(0);
  }
  else if(F==1)  {
    secs = (clock() - ut0)/(double)CLOCKS_PER_SEC;
    if (secs<0) secs=0;	/* clock() can overflow. */
    sprintf(s, "User time: %8.0lf sec = %8.2lf hr.  %s\n", 
	    secs, secs/3600.0, Msg);
    puts(s);
    strcpy(Msg, s);
    return(difftime(time(NULL), rt0));
  }
  else if(F==2) return(difftime(time(NULL), rt0));
  else return(0);
#endif
}

/***********************************************************
 *	Print the current time and the estimated finishing time.
 *
 *	P1 is the number of computed photon packets.
 *	Pt is the total number of photon packets.
 ****/
void PredictDoneTime(long P1, long Pt)	
{
  time_t now, done_time;
  struct tm *date;
  char s[80];
  
  now = time(NULL);
  date = localtime(&now);
  strftime(s, 80, "%H:%M %x", date);
  printf("Now %s, ", s);
  
  done_time = now + 
			(time_t) (PunchTime(2,"")/(double)P1*(Pt-P1));
  date = localtime(&done_time);
  strftime(s, 80, "%H:%M %x", date);
  printf("End %s\n", s);
}

/***********************************************************
 *	Report time and write results. 
 ****/
void ReportResult(InputStruct In_Parm, OutStruct Out_Parm)
{
  char time_report[STRLEN];
  
  strcpy(time_report, " Simulation time of this run.");
  PunchTime(1, time_report);

  SumScaleResult(In_Parm, &Out_Parm);
  WriteResult(In_Parm, Out_Parm, time_report);
}

/***********************************************************
 *	Get the file name of the input data file from the 
 *	argument to the command line.
 ****/
void GetFnameFromArgv(int argc,
					  char * argv[],
					  char * input_filename)
{
  if(argc>=2) {			/* filename in command line */
    strcpy(input_filename, argv[1]);
  }
  else
    input_filename[0] = '\0';
} 

    
/***********************************************************
 *	Execute Monte Carlo simulation for one independent run.
 ****/
void DoOneRun(short NumRuns, InputStruct *In_Ptr, MultiScatterStruct multi)
{
  register long i_photon;	
	/* index to photon. register for speed.*/
  OutStruct out_parm;		/* distribution of photons.*/
  PhotonStruct photon;
  long num_photons = In_Ptr->num_photons, photon_rep=10;

#if THINKCPROFILER
  InitProfile(200,200); cecho2file("prof.rpt",0, stdout);
#endif
    
  InitOutputData(*In_Ptr, &out_parm);
  out_parm.Rsp = Rspecular(In_Ptr->layerspecs);	
  i_photon = num_photons;
  PunchTime(0, "");
    
  do {
    if(num_photons - i_photon == photon_rep) {
      printf("%ld photons & %hd runs left, ", i_photon, NumRuns);
      PredictDoneTime(num_photons - i_photon, num_photons);
      photon_rep *= 10;
    }
    LaunchPhoton(out_parm.Rsp, In_Ptr->layerspecs, &photon);
    do  HopDropSpin(In_Ptr, &photon, &out_parm, multi);
    while (!photon.dead);
  } while(--i_photon);
    
#if THINKCPROFILER
  exit(0);
#endif
    
  ReportResult(*In_Ptr, out_parm);
  FreeData(*In_Ptr, &out_parm);
}

void readAnisotropy(MultiScatterStruct *Multiptr) {

  FILE anisotropyInput = *fopen("anisotropyInput.txt","r");
  char buffer1[1000];
  fgets(buffer1, 1000, &anisotropyInput);
  double diameter[40];
  char* temp1 = strtok(buffer1, ",");
  diameter[0] = atoi(temp1);
  for (int counter1 = 1; counter1 < 40; counter1++){
    temp1 = strtok (NULL, " ,.-");
    diameter[counter1] = atoi(temp1);
  }

  char buffer2[1000];
  fgets(buffer2, 1000, &anisotropyInput);
  
  double percentages[40];
  char* temp2 = strtok(buffer2, ",");
  percentages[0] = atoi(temp2);
  for (int counter2 = 1; counter2 < 40; counter2++){
    temp2 = strtok (NULL, " ,.-");
    percentages[counter2] = atoi(temp2);
  }

  char buffer3[10000];
  fgets(buffer3, 10000, &anisotropyInput);
  
  double anisotropy[40];
  char* temp3 = strtok(buffer3, ",");
  anisotropy[0] = atof(temp3);
  for (int counter3 = 1; counter3 < 40; counter3++){
    temp3 = strtok (NULL, ",");
    anisotropy[counter3] = atof(temp3);
    Multiptr->anisotropyArray[counter3] = anisotropy[counter3];
  }

  char buffer4[1000];
  fgets(buffer4, 1000, &anisotropyInput);
  Multiptr->l_s = atoi(buffer4);
 

  double volumes[40];
  for (int counter4 = 0; counter4 < 40; counter4++) {
    volumes[counter4] = pow(diameter[counter4], 3) / 6 * PI; 
  }
  

  double percentageByVolume[40];
  double totalVolume = 0;
  for (int counter5 = 0; counter5 < 40; counter5++) {
    percentageByVolume[counter5] = (volumes[counter5] * percentages[counter5]);
    totalVolume += percentageByVolume[counter5];
  }

  double percentageHit[40];
  for (int counter6 = 0; counter6 < 40; counter6++) {
    percentageHit[counter6] = percentageByVolume[counter6] / totalVolume;
    Multiptr->percentagesHit[counter6] = percentageHit[counter6];
  }
 
  fclose(&anisotropyInput); 
}



/***********************************************************
 *	The argument to the command line is filename, if any.
 *	Macintosh does not support command line.
 ****/
char main(int argc, char *argv[]) 
{
  char input_filename[STRLEN];
  FILE *input_file_ptr;
  short num_runs;	/* number of independent runs. */
  InputStruct in_parm;
  MultiScatterStruct multi;
  readAnisotropy(&multi);
  printf("%f",multi.anisotropyArray[39]);
  ShowVersion("Version 1.2, 1993");
  GetFnameFromArgv(argc, argv, input_filename);
  input_file_ptr = GetFile(input_filename);
  CheckParm(input_file_ptr, &in_parm);	
  num_runs = ReadNumRuns(input_file_ptr);
  srand(time(NULL));
  while(num_runs--)  {
    ReadParm(input_file_ptr, &in_parm);
	 DoOneRun(num_runs, &in_parm, multi);
  }
  
  fclose(input_file_ptr);
  return(0);
}

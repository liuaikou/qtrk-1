#include "../cputrack/cpu_tracker.h"
#include <Windows.h>
#include <stdint.h>
#include "../cputrack/random_distr.h"
#include "../cputrack/queued_cpu_tracker.h"

template<typename T> T sq(T x) { return x*x; }
template<typename T> T distance(T x, T y) { return sqrt(x*x+y*y); }




double getPreciseTime()
{
	uint64_t freq, time;

	QueryPerformanceCounter((LARGE_INTEGER*)&time);
	QueryPerformanceFrequency((LARGE_INTEGER*)&freq);

	return (double)time / (double)freq;
}


void SpeedTest()
{
#ifdef _DEBUG
	int N = 20;
#else
	int N = 1000;
#endif
	int qi_iterations = 4;
	int xcor_iterations = 2;
	CPUTracker* tracker = new CPUTracker(150,150, 128);

	int radialSteps = 64, zplanes = 120;
	float zmin = 2, zmax = 8;
	float zradius = tracker->xcorw/2;

	float* zlut = new float[radialSteps*zplanes];
	for (int x=0;x<zplanes;x++)  {
		vector2f center = { tracker->GetWidth()/2, tracker->GetHeight()/2 };
		float s = zmin + (zmax-zmin) * x/(float)(zplanes-1);
		GenerateTestImage(ImageData(tracker->srcImage, tracker->GetWidth(), tracker->GetHeight()), center.x, center.y, s, 0.0f);
		tracker->ComputeRadialProfile(&zlut[x*radialSteps], radialSteps, 64, 1, zradius, center);	
	}
	tracker->SetZLUT(zlut, zplanes, radialSteps, 1,1, zradius, 64, true, true);
	delete[] zlut;

	// Speed test
	vector2f comdist={}, xcordist={}, qidist={};
	float zdist=0.0f;
	double zerrsum=0.0f;
	double tcom = 0.0, tgen=0.0, tz = 0.0, tqi=0.0, txcor=0.0;
	for (int k=0;k<N;k++)
	{
		double t0 = getPreciseTime();
		float xp = tracker->GetWidth()/2+(rand_uniform<float>() - 0.5) * 5;
		float yp = tracker->GetHeight()/2+(rand_uniform<float>() - 0.5) * 5;
		float z = zmin + 0.1f + (zmax-zmin-0.2f) * rand_uniform<float>();

		GenerateTestImage(ImageData(tracker->srcImage, tracker->GetWidth(), tracker->GetHeight()), xp, yp, z, 10000);

		double t1 = getPreciseTime();
		vector2f com = tracker->ComputeBgCorrectedCOM();
		vector2f initial = {com.x, com.y};
		double t2 = getPreciseTime();
		bool boundaryHit = false;
		vector2f xcor = tracker->ComputeXCorInterpolated(initial, xcor_iterations, 32, boundaryHit);
/*		if (k == 1) {
			tracker.OutputDebugInfo();
			writeImageAsCSV("test.csv", tracker.srcImage, tracker.width, tracker.height);
		}*/

		comdist.x += fabsf(com.x - xp);
		comdist.y += fabsf(com.y - yp);

		xcordist.x += fabsf(xcor.x - xp);
		xcordist.y += fabsf(xcor.y - yp);
		double t3 = getPreciseTime();
		boundaryHit = false;
		vector2f qi = tracker->ComputeQI(xcor, qi_iterations, 64, 16, 5,50, boundaryHit);
		qidist.x += fabsf(qi.x - xp);
		qidist.y += fabsf(qi.y - yp);
		double t4 = getPreciseTime();

		float est_z = zmin + (zmax-zmin)*tracker->ComputeZ(xcor, 64, 0, &boundaryHit, 0) / (zplanes-1);
		zdist += fabsf(est_z-z);
		zerrsum += est_z-z;

		double t5 = getPreciseTime();
	//	dbgout(SPrintf("xpos:%f, COM err: %f, XCor err: %f\n", xp, com.x-xp, xcor.x-xp));
		if (k>0) { // skip first initialization round
			tgen+=t1-t0;
			tcom+=t2-t1;
			txcor+=t3-t2;
			tqi+=t4-t3;
			tz+=t5-t4;
		}
		if (boundaryHit)
			dbgprintf("boundaryhit!!\n");
	}

	int Nns = N-1;
	dbgprintf("Image gen. (img/s): %f\nCenter-of-Mass speed (img/s): %f\n", Nns/tgen, Nns/tcom);
	dbgprintf("XCor estimation (img*it/s): %f\n", (Nns*xcor_iterations)/txcor);
	dbgprintf("COM+XCor(%d) (img/s): %f\n", xcor_iterations, Nns/(tcom+txcor));
	dbgprintf("Z estimation (img/s): %f\n", Nns/tz);
	dbgprintf("QI speed: %f (img*it/s)\n", (Nns*qi_iterations)/tqi);
	dbgprintf("Average dist: COM x: %f, y: %f\n", comdist.x/N, comdist.y/N);
	dbgprintf("Average dist: Cross-correlation x: %f, y: %f\n", xcordist.x/N, xcordist.y/N);
	dbgprintf("Average dist: QI x: %f, y: %f\n", qidist.x/N, qidist.y/N);
	dbgprintf("Average dist: Z: %f. Mean error:%f\n", zdist/N, zerrsum/N); 
	
	delete tracker;
}

void OnePixelTest()
{
	CPUTracker* tracker = new CPUTracker(32,32, 16);

	tracker->getPixel(15,15) = 1;
	dbgout(SPrintf("Pixel at 15,15\n"));
	vector2f com = tracker->ComputeBgCorrectedCOM();
	dbgout(SPrintf("COM: %f,%f\n", com.x, com.y));
	
	vector2f initial = {15,15};
	bool boundaryHit = false;
	vector2f xcor = tracker->ComputeXCorInterpolated(initial,2, 16, boundaryHit);
	dbgout(SPrintf("XCor: %f,%f\n", xcor.x, xcor.y));

	assert(xcor.x == 15.0f && xcor.y == 15.0f);
	delete tracker;
}
 
void SmallImageTest()
{
	CPUTracker *tracker = new CPUTracker(32,32, 16);

	GenerateTestImage(ImageData(tracker->srcImage, tracker->GetWidth(), tracker->GetHeight()), 15,15, 1, 0.0f);

	vector2f com = tracker->ComputeBgCorrectedCOM();
	dbgout(SPrintf("COM: %f,%f\n", com.x, com.y));
	
	vector2f initial = {15,15};
	bool boundaryHit = false;
	vector2f xcor = tracker->ComputeXCorInterpolated(initial, 2, 16, boundaryHit);
	dbgout(SPrintf("XCor: %f,%f\n", xcor.x, xcor.y));

	assert(fabsf(xcor.x-15.0f) < 1e-6 && fabsf(xcor.y-15.0f) < 1e-6);
	delete tracker;
}


 
void OutputProfileImg()
{
	CPUTracker *tracker = new CPUTracker(128,128, 16);
	bool boundaryHit;

	for (int i=0;i<10;i++) {
		float xp = tracker->GetWidth()/2+(rand_uniform<float>() - 0.5) * 20;
		float yp = tracker->GetHeight()/2+(rand_uniform<float>() - 0.5) * 20;
		
		GenerateTestImage(ImageData(tracker->srcImage, tracker->GetWidth(), tracker->GetHeight()), xp, yp, 1, 0.0f);

		vector2f com = tracker->ComputeBgCorrectedCOM();
		dbgout(SPrintf("COM: %f,%f\n", com.x-xp, com.y-yp));
	
		vector2f initial = com;
		boundaryHit=false;
		vector2f xcor = tracker->ComputeXCorInterpolated(initial, 3, 16, boundaryHit);
		dbgprintf("XCor: %f,%f. Err: %d\n", xcor.x-xp, xcor.y-yp, boundaryHit);

		boundaryHit=false;
		vector2f qi = tracker->ComputeQI(initial, 3, 64, 32, 1, 10, boundaryHit);
		dbgprintf("QI: %f,%f. Err: %d\n", qi.x-xp, qi.y-yp, boundaryHit);
	}

	delete tracker;
}


 
void TestBoundCheck()
{
	CPUTracker *tracker = new CPUTracker(32,32, 16);
	bool boundaryHit;

	for (int i=0;i<10;i++) {
		float xp = tracker->GetWidth()/2+(rand_uniform<float>() - 0.5) * 20;
		float yp = tracker->GetHeight()/2+(rand_uniform<float>() - 0.5) * 20;
		
		GenerateTestImage(ImageData(tracker->srcImage, tracker->GetWidth(), tracker->GetHeight()), xp, yp, 1, 0.0f);

		vector2f com = tracker->ComputeBgCorrectedCOM();
		dbgout(SPrintf("COM: %f,%f\n", com.x-xp, com.y-yp));
	
		vector2f initial = com;
		boundaryHit=false;
		vector2f xcor = tracker->ComputeXCorInterpolated(initial, 3, 16, boundaryHit);
		dbgprintf("XCor: %f,%f. Err: %d\n", xcor.x-xp, xcor.y-yp, boundaryHit);

		boundaryHit=false;
		vector2f qi = tracker->ComputeQI(initial, 3, 64, 32, 1, 10, boundaryHit);
		dbgprintf("QI: %f,%f. Err: %d\n", qi.x-xp, qi.y-yp, boundaryHit);
	}

	delete tracker;
}


void PixelationErrorTest()
{
	CPUTracker *tracker = new CPUTracker(128,128, 64);

	float X = tracker->GetWidth()/2;
	float Y = tracker->GetHeight()/2;
	int N = 20;
	for (int x=0;x<N;x++)  {
		float xpos = X + 2.0f * x / (float)N;
		GenerateTestImage(ImageData(tracker->srcImage, tracker->GetWidth(), tracker->GetHeight()), xpos, X, 1, 0.0f);

		vector2f com = tracker->ComputeBgCorrectedCOM();
		//dbgout(SPrintf("COM: %f,%f\n", com.x, com.y));

		vector2f initial = {X,Y};
		bool boundaryHit = false;
		vector2f xcorInterp = tracker->ComputeXCorInterpolated(initial, 3, 32, boundaryHit);
		dbgout(SPrintf("xpos:%f, COM err: %f, XCorInterp err: %f\n", xpos, com.x-xpos, xcorInterp.x-xpos));
	}
	delete tracker;
}

float EstimateZError(int zplanes)
{
	// build LUT
	CPUTracker *tracker = new CPUTracker(128,128, 64);

	vector2f center = { tracker->GetWidth()/2, tracker->GetHeight()/2 };
	int radialSteps = 64;
	float* zlut = new float[radialSteps*zplanes];
	float zmin = 2, zmax = 8;
	float zradius = tracker->xcorw/2;

	//GenerateTestImage(&tracker, center.x, center.y, zmin, 0.0f);
	//writeImageAsCSV("img.csv", tracker.srcImage, tracker.width, tracker.height);

	for (int x=0;x<zplanes;x++)  {
		float s = zmin + (zmax-zmin) * x/(float)(zplanes-1);
		GenerateTestImage(ImageData(tracker->srcImage, tracker->GetWidth(), tracker->GetHeight()), center.x, center.y, s, 0.0f);
	//	dbgout(SPrintf("z=%f\n", s));
		tracker->ComputeRadialProfile(&zlut[x*radialSteps], radialSteps, 64, 1.0f, zradius, center);
	}

	tracker->SetZLUT(zlut, zplanes, radialSteps, 1, 1.0f, zradius, 64, true, true);
	WriteImageAsCSV("zlut.csv", zlut, radialSteps, zplanes);
	delete[] zlut;

	int N=100;
	float zdist=0.0f;
	std::vector<float> cmpProf;
	for (int k=0;k<N;k++) {
		float z = zmin + k/float(N-1) * (zmax-zmin);
		GenerateTestImage(ImageData(tracker->srcImage, tracker->GetWidth(), tracker->GetHeight()), center.x, center.y, z, 0.0f);
		
		float est_z = zmin + tracker->ComputeZ(center, 64, 0, 0, 0, 0);
		zdist += fabsf(est_z-z);
		//dbgout(SPrintf("Z: %f, EstZ: %f\n", z, est_z));

		if(k==50) {
			WriteImageAsCSV("rprofdiff.csv", &cmpProf[0], cmpProf.size(),1);
		}
	}
	return zdist/N;
}


void ZTrackingTest()
{
	for (int k=20;k<100;k+=20)
	{
		float err = EstimateZError(k);
		dbgout(SPrintf("average Z difference: %f. zplanes=%d\n", err, k));
	}
}

/*
void Test2DTracking()
{
	CPUTracker tracker(150,150);

	float zmin = 2;
	float zmax = 6;
	int N = 200;

	double tloc2D = 0, tloc1D = 0;
	double dist2D = 0;
	double dist1D = 0;
	for (int k=0;k<N;k++) {
		float xp = tracker.GetWidth()/2+(rand_uniform<float>() - 0.5) * 5;
		float yp = tracker.GetHeight()/2+(rand_uniform<float>() - 0.5) * 5;
		float z = zmin + 0.1f + (zmax-zmin-0.2f) * rand_uniform<float>();

		GenerateTestImage(tracker.srcImage, tracker.GetWidth(), tracker.GetHeight(), xp, yp, z, 50000);

		double t0 = getPreciseTime();
		vector2f xcor2D = tracker.ComputeXCor2D();
		if (k==0) {
			float * results = tracker.tracker2D->GetAutoConvResults();
			writeImageAsCSV("xcor2d-autoconv-img.csv", results, tracker.GetWidth(), tracker.GetHeight());
		}

		double t1 = getPreciseTime();
		vector2f com = tracker.ComputeBgCorrectedCOM();
		vector2f xcor1D = tracker.ComputeXCorInterpolated(com, 2);
		double t2 = getPreciseTime();

		dist1D += distance(xp-xcor1D.x,yp-xcor1D.y);
		dist2D += distance(xp-xcor2D.x,yp-xcor2D.y);

		if (k>0) {
			tloc2D += t1-t0;
			tloc1D += t2-t1;
		}
	}
	N--; // ignore first

	dbgprintf("1D Xcor speed(img/s): %f\n2D Xcor speed (img/s): %f\n", N/tloc1D, N/tloc2D);
	dbgprintf("Average dist XCor 1D: %f\n", dist1D/N);
	dbgprintf("Average dist XCor 2D: %f\n", dist2D/N);
}*/

void QTrkTest()
{
	QTrkSettings cfg;
	cfg.width = cfg.height = 128;
	cfg.qi_iterations = 3;
	cfg.qi_maxradius = 50;
	cfg.xc1_iterations = 2;
	cfg.xc1_profileLength = 64;
	//cfg.numThreads = 6;
	QueuedCPUTracker qtrk(&cfg);
	float *image = new float[cfg.width*cfg.height];

	// Generate ZLUT
	int radialSteps=64, zplanes=100;
	float zmin=2,zmax=6;
	float* zlut = new float[radialSteps*zplanes];
	for (int x=0;x<zplanes;x++)  {
		vector2f center = { cfg.width/2, cfg.height/2 };
		float s = zmin + (zmax-zmin) * x/(float)(zplanes-1);
		GenerateTestImage(ImageData(image, cfg.width, cfg.height), center.x, center.y, s, 0.0f);
		qtrk.ComputeRadialProfile(image, cfg.width, cfg.height, &zlut[x*radialSteps], radialSteps, center);
	}
	qtrk.SetZLUT(zlut, zplanes, radialSteps, 1);
	delete[] zlut;

	// Schedule images to localize on
#ifdef _DEBUG
	int NumImages=10, JobsPerImg=10;
#else
	int NumImages=10, JobsPerImg=1000;
#endif
	dbgprintf("Generating %d images...\n", NumImages);
	double tgen = 0.0, tschedule = 0.0;
	std::vector<float> truepos(NumImages*JobsPerImg*3);
	for (int n=0;n<NumImages;n++) {
		double t1 = getPreciseTime();
		float xp = cfg.width/2+(rand_uniform<float>() - 0.5) * 5;
		float yp = cfg.height/2+(rand_uniform<float>() - 0.5) * 5;
		float z = zmin + 0.1f + (zmax-zmin-0.2f) * rand_uniform<float>();
		truepos[n*3+0] = xp;
		truepos[n*3+1] = yp;
		truepos[n*3+2] = z;

		GenerateTestImage(ImageData(image, cfg.width, cfg.height), xp, yp, z, 10000);
		double t2 = getPreciseTime();
		for (int k=0;k<JobsPerImg;k++)
			qtrk.ScheduleLocalization((uchar*)image, cfg.width*sizeof(float), QTrkFloat, (LocalizeType)(LocalizeXCor1D), n*JobsPerImg+k, 0);
		double t3 = getPreciseTime();
		tgen += t2-t1;
		tschedule += t3-t2;
	}
	delete[] image;
	dbgprintf("Schedule time: %f, Generation time: %f\n", tschedule, tgen);

	// Measure speed
	dbgprintf("Localizing on %d images...\n", NumImages*JobsPerImg);
	double tstart = getPreciseTime();
	int jobc = 0;
	int hjobc = qtrk.GetJobCount();
	int startJobs = hjobc;
	qtrk.Start();
	do {
		jobc = qtrk.GetJobCount();
		while (hjobc>jobc) {
			if( hjobc%100==0) dbgprintf("TODO: %d\n", hjobc);
			hjobc--;
		}
		Sleep(10);
	} while (jobc!=0);
	double tend = getPreciseTime();

	// Wait for last jobs
	jobc = NumImages*JobsPerImg;
	int rc = jobc;
	double errX=0.0, errY=0.0, errZ=0.0;

	while(rc>0) {
		LocalizationResult result;

		if (qtrk.PollFinished(&result, 1)) {
			int iid = result.id/JobsPerImg;
			errX += fabs(truepos[iid*3+0]-result.pos.x);
			errY += fabs(truepos[iid*3+1]-result.pos.y);
			errZ += fabs(truepos[iid*3+2]-result.z);
			dbgprintf("ID: %d. Error:%d\n", result.id, result.error);
			rc--;
		}
	}
	dbgprintf("Localization Speed: %d (img/s), using %d threads\n", (int)( startJobs/(tend-tstart) ), qtrk.NumThreads());
	dbgprintf("ErrX: %f, ErrY: %f, ErrZ: %f\n", errX/jobc, errY/jobc,errZ/jobc);
}


int main()
{
	SpeedTest();
	//SmallImageTest();
	//PixelationErrorTest();
	//ZTrackingTest();
	//Test2DTracking();
	//TestBoundCheck();
	QTrkTest();

	return 0;
}

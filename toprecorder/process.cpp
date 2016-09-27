#include <stdio.h>
#include <vector>
#include <string>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <map>


using namespace std;

#define NRNODES 5

struct DataEntry
{
	string IP;
	int rssi;
	string macFrom;
	string macTo;
	uint32_t pid;
	uint32_t time;
};

vector< DataEntry > Entries;

class Node;

Node * NODES[NRNODES];


void LoadEntries( const char * filename );
int gFrame;
int still_init = 0;
const double MHz = 160; //160;
const double SoL = 299792458;
double CalculateToF( string Mac1, string Mac2 ); //In 1/MHz
int GetNodeId( string mac );


/*	NODES[0] = new Node( "5ccf7fc0c75c", 1, 0, 0, 0, 0);  //.147
	NODES[1] = new Node( "5ccf7fc0d218", 1, DistanceMeters168, 0, 0, 1 );  //.179
	NODES[2] = new Node( "5ccf7fc10b08", 1, 0, DistanceMeters162, 0, 2 );  //.169
	NODES[3] = new Node( "5ccf7fc06055", 1, DistanceMeters168, DistanceMeters162, 0, 3 );  //.241
*/

#define KNOWN_NODE_3 1

#if 1
double ToFMatrix[5][5] = {
	0, 2.2, 2.2, 3.17, 1.5, 
	2.2, 0, 3.17, 2.2, 1.5,
	2.2, 3.17, 0, 2.2, 1.5,
	3.17, 2.2, 2.2, 0, 1.5,
	 1.5, 1.5, 1.5, 1.5, 0 };
#else
#if 0
double ToFMatrix[5][5] = {
	0, 2.2, 2.2, 3.17,
	2.2, 0, 3.17, 2.2,
	2.2, 3.17, 0, 2.2,
	3.17, 2.2, 2.2, 0,
	};
#else
double ToFMatrix[5][5] = {
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	0, 0, 0, 0,
	};
#endif
#endif

//#define NR_ITERATIONS 10000
#define NR_ITERATIONS 1
#define INIT_FRAMES 2000
#define START_CULL  7000
#define DETAIL_DEBUG

#ifdef DETAIL_DEBUG
#undef NR_ITERATIONS
#define NR_ITERATIONS 1
#endif

static const double syncoff = 0.001; //The factor by which to correct off-sync-ness
static const double syncslew = 0.0000002; //The factor by which to correct slew offsets
static const double syncslewD = 0.00001;

static const double syncoff_before = 0.1; //The factor by which to correct off-sync-ness
static const double syncslew_before = 0.0000001; //The factor by which to correct slew offsets


class Node
{
public:

	Node() : myMac(""), is_known(false), x(0), y(0), z(0), nodeid(-1), currentpid(-1), VirtualTimeOffset(0), DeltaTFilt(0), LastDeltaTFilt(0), got_time(0), last_time(0), running_time(0) {
		memset( GotPeerTime, 0, sizeof( GotPeerTime ) ); memset( PeerTime, 0, sizeof( PeerTime ) ); memset( othernodedeltas, 0, sizeof( othernodedeltas ) ); memset( othernodedeltascount, 0, sizeof(othernodedeltascount) ); memset( LastDelta, 0, sizeof( LastDelta ) );
	}

	Node( string mac, int ik, double tx, double ty, double tz, int nid ) : myMac( mac ), is_known(ik), x(tx), y(ty), z(tz), nodeid(nid), VirtualTimeOffset(0), DeltaTFilt(0), LastDeltaTFilt(0), got_time(0), last_time(0), running_time(0) {
		memset( GotPeerTime, 0, sizeof( GotPeerTime ) ); memset( PeerTime, 0, sizeof( PeerTime ) ); memset( othernodedeltas, 0, sizeof( othernodedeltas ) ); memset( othernodedeltascount, 0, sizeof(othernodedeltascount) ); memset( LastDelta, 0, sizeof( LastDelta ) );
	}

	string myMac;
	int is_known; //Is the ToFs of this node known?
	double x;
	double y;
	double z;

	int nodeid;
	int currentpid;

	bool GotPeerTime[NRNODES];
	double PeerTime[NRNODES];
	double LastDelta[NRNODES];

	//Tricky: This is the time sync values (will be set by sender on receiver)
	double VirtualTimeOffset;
	double DeltaTFilt;
	double LastDeltaTFilt;

	//These three are just for parsing times, nothing tricky.
	double running_time;
	double clockskewratio;
	uint32_t last_time;
	bool got_time;

	int othernodedeltascount[NRNODES];
	double othernodedeltas[NRNODES];

	//Called by transmitters.  Once per
	void SyncTimes( int nodeto )
	{
		int i, j;
//		if( is_known )
		{
			for( i = 0; i < NRNODES; i++ )
			{
				if( nodeto == i ) continue;
				
				if( GotPeerTime[i] )
				{
					double delta = PeerTime[nodeto] - PeerTime[i];
					//LastDelta[i] = delta;

					//printf( "%f - %f = %f\n", PeerTime[nodeto], PeerTime[i], delta );
					if( still_init )
					{
						if( delta < -100000 ) delta = -100000;
						if( delta > 100000 ) delta = 100000;
					}
					else
					{
						//fprintf( stderr, "%f", delta );

						
						//If the delta is unacceptable (60 seems good) that means we got a wrong timestamp.  Throw it out.
						if( gFrame > START_CULL*2 )
						{
							if( delta < -8 ) continue; //delta = -20;
							if( delta > 8 ) continue; //delta = 20;
							NODES[i]->LastDelta[nodeto] = delta;
							NODES[nodeto]->LastDelta[i] = -delta;
						}
						else if( gFrame > START_CULL )
						{
							if( delta < -50 ) continue; //delta = -20;
							if( delta > 50 ) continue; //delta = 20;
							//fprintf( stderr, "\n", delta );
							NODES[i]->LastDelta[nodeto] = delta;
							NODES[nodeto]->LastDelta[i] = -delta;
						}


					}

					if( gFrame > START_CULL )
					{
						othernodedeltas[nodeto] += delta;
						othernodedeltascount[nodeto]++;
						othernodedeltas[i] += -delta;
						othernodedeltascount[i]++;
						//printf( "%d / %d / %d\n", nodeid, othernode, othernodedeltascount[othernode] );
					}


					if( NODES[nodeto]->is_known )
					{
						NODES[i]->UpdateVirtualTime( delta, nodeto );
					}
					if( NODES[i]->is_known )
					{
						NODES[nodeto]->UpdateVirtualTime( -delta, i );
					}
				}
			}

#ifdef DETAIL_DEBUG
			//if( !still_init )
			{
				for( i = 0; i < NRNODES; i++ )
				{
					printf( "%f, %f, %f, %f, %f, %f, %f, ", NODES[i]->VirtualTimeOffset, (NODES[i]->clockskewratio-1.0)*1000000, NODES[i]->LastDelta[0], NODES[i]->LastDelta[1], NODES[i]->LastDelta[2], NODES[i]->LastDelta[3], NODES[i]->LastDelta[4] );
	//				printf( "%f ", NODES[i]->VirtualTimeOffset );
				}
				printf( "%d\n", still_init );
			}
#endif
		}
	}

	void UpdateVirtualTime( double delta, int othernode )
	{
		if( still_init )
			delta = delta * syncoff_before;
		else
			delta = delta * syncoff;

		double tsync = syncslew;
		if( still_init ) tsync = syncslew_before;

		double clockskewoff = tsync * delta;
		if( clockskewoff > tsync ) clockskewoff = tsync;
		if( clockskewoff <-tsync ) clockskewoff =-tsync;
		clockskewratio += clockskewoff;
		VirtualTimeOffset += delta;

		DeltaTFilt = DeltaTFilt * .99 + delta * .1;

		clockskewratio += syncslewD * -(LastDeltaTFilt-DeltaTFilt);
		
		LastDeltaTFilt = DeltaTFilt;
	}

	void ResetPeerTimes()
	{
		for( int i = 0; i < NRNODES; i++ )
		{
			GotPeerTime[i] = 0;
			PeerTime[i] = 0;
		}
	}

	//This is called when a foreign object received one of our packets.
	void TransmittedMessage( int nodeto, double RealTime, int pid )
	{
		if( nodeto < 0 ) return;

		if( pid != currentpid )
		{
			//Clear out adjacency properties.
			currentpid = pid;
			ResetPeerTimes();
		}

		GotPeerTime[nodeto] = 1;
		PeerTime[nodeto] = (RealTime - GetToF( *NODES[nodeto] ));
		SyncTimes( nodeto );
	}

	double GetToF( Node & other )										///TOF TOF TOF
	{
		if ( !is_known || !other.is_known ) return 0;
		//return ToFMatrix[nodeid][other.nodeid]*1.0;
		double dx = x - other.x;
		double dy = y - other.y;
		double dz = z - other.z;
		double dist = sqrtf( dx*dx+dy*dy+dz*dz );
		double dt = dist / SoL * (MHz * 1000000); 
		return dt;

	}

	double RxTimeToReal( uint32_t rxtime )  //In ticks.
	{
		if( !got_time )
		{
			got_time = true;
			last_time = rxtime;
			running_time = 0;
			clockskewratio = 1.0;
			return 0;
		}

		uint32_t rxdtime = rxtime - last_time;
		last_time = rxtime;
		running_time += rxdtime * clockskewratio;
		return running_time + VirtualTimeOffset; //In ticks
	}
};


void ProcessEntries()
{
	int i;

	for( i = 0; i < Entries.size(); i++ )
	{
		DataEntry & e = Entries[i];
		//192.168.11.169/44/5ccf7fc0c75c/5ccf7fc10b08/18079/2194663737
		//printf ("%s/%d/%s/%s/%u/%u\n", e.IP.c_str(), e.rssi, e.macFrom.c_str(), e.macTo.c_str(), e.pid, e.time );

		if( e.time == 0 ) continue;

		gFrame = i;
		still_init = i < INIT_FRAMES; //If still init, update clocks very quickly.
		int nfrom = GetNodeId( e.macFrom );
		int nto = GetNodeId( e.macTo );

		if( nto >= 0 && nfrom >= 0 )
		{
			double LoopTime = NODES[nto]->RxTimeToReal( e.time );
			NODES[nfrom]->TransmittedMessage( nto, LoopTime, e.pid );
		}
	}

/*
	double SlaveTimeDeltas[3];
	double SlaveTimeDeltasTemp[3];
	int    GotNodesTemp[3];
	int    CurSyncId;

	double slews[2];
	int firstslew = 0;

	for( i = 0; i < Entries.size(); i++ )
	{
		DataEntry & e = Entries[i];
		//192.168.11.169/44/5ccf7fc0c75c/5ccf7fc10b08/18079/2194663737
		double tof = CalculateToF( e.macFrom, e.macTo );
		//printf ("%s/%d/%s/%s/%u/%u/%f\n", e.IP.c_str(), e.rssi, e.macFrom.c_str(), e.macTo.c_str(), e.pid, e.time, tof );

		int nodeto = GetNodeId( e.macTo ) - 1;
		if( e.pid != CurSyncId )
		{
			CurSyncId = e.pid;
			GotNodesTemp[0] = GotNodesTemp[1] = GotNodesTemp[2] = 0;
		}

		if( e.macFrom == MasterMac )
		{
			double sendtime = e.time - tof;
			SlaveTimeDeltasTemp[nodeto] = sendtime;
			GotNodesTemp[nodeto] = 1;
			if( GotNodesTemp[0] && GotNodesTemp[1] && GotNodesTemp[2] )
			{
				SlaveTimeDeltas[0] = SlaveTimeDeltasTemp[0];
				SlaveTimeDeltas[1] = SlaveTimeDeltasTemp[1];
				SlaveTimeDeltas[2] = SlaveTimeDeltasTemp[2];
				uint32_t slew1 = SlaveTimeDeltas[0] - SlaveTimeDeltasTemp[2];
				uint32_t slew2 = SlaveTimeDeltas[1] - SlaveTimeDeltasTemp[2];
				if( firstslew == 0 )
				{
					firstslew = 1;
					slews[0] = slew1;
					slews[1] = slew2;
				}
				printf( "%f %f\n", slew1 - slews[0], slew2 - slews[1] );
			}
		}
	}*/
}


int main( int argc, char ** argv )
{
	if( argc != 2 )
	{
		fprintf( stderr, "Error: Usage: [tool] [data file]\n" );
		return -5;
	}


	//192.168.11.169 27102e400000 5ccf7fc06055 5ccf7fc10b08 ESPEEDEE 17879 4255185525
	LoadEntries( argv[1] );


	double BackupToFMatrix[5][5];
	double backupError = 1e20;
	double anneal = 1.0;
		memcpy(BackupToFMatrix, ToFMatrix, sizeof( ToFMatrix ) );

	srand(5);
	int k;
	for (k = 0; k < NR_ITERATIONS; k++)
	{

		if( NR_ITERATIONS > 1 )
		{
			memcpy( ToFMatrix, BackupToFMatrix, sizeof( ToFMatrix ) );
			int xcellmod = rand()%(NRNODES);
			int ycellmod = rand()%(NRNODES-1);
			if( ycellmod >= xcellmod ) ycellmod++;
			//ToFMatrix[xcellmod][ycellmod] += (rand()%10000-5000)/5000.0*anneal;
			double emt = (rand()%10000-5000)/5000.0*anneal;
			ToFMatrix[xcellmod][ycellmod] += emt;
			ToFMatrix[ycellmod][xcellmod] += emt;

			xcellmod = rand()%NRNODES;
			ycellmod = rand()%(NRNODES-1);
			if( ycellmod >= xcellmod ) ycellmod++;
			emt = (rand()%10000-5000)/5000.0*anneal;
			//ToFMatrix[xcellmod][ycellmod] += (rand()%10000-5000)/5000.0*anneal;
			ToFMatrix[xcellmod][ycellmod] += emt;
			ToFMatrix[ycellmod][xcellmod] += emt;
		}

		anneal *= 0.9995;
		//ToFMatrix[ycellmod][xcellmod] += (rand()%10000)/10000.0;

		int i;
		for( i = 0; i < NRNODES; i++ )
			if( NODES[i] ) delete NODES[i];

		double DistanceMeters162 = -4.1148*1.0;
		double DistanceMeters168 = 4.2672*1.0;

	#define DO_SPACING

/*
	#ifdef DO_SPACING
		NODES[0] = new Node( "5ccf7fc0c75c", 1, 			0, 0, 0, 0);  //.147
		NODES[1] = new Node( "5ccf7fc0d218", 1, 			DistanceMeters168, DistanceMeters162, 0, 1 );  //.179
		NODES[2] = new Node( "5ccf7fc10b08", 1,				0, DistanceMeters162, 0, 2 );  //.169(167)
		NODES[3] = new Node( "5ccf7fc06055", KNOWN_NODE_3, 	DistanceMeters168, 0, 0, 3 );  //.241
	#if NRNODES>4
		NODES[4] = new Node( "5ccf7fc10aff", 1, DistanceMeters168/2, DistanceMeters162/2, 0, 3 );  //.214
	#endif

	#else
		NODES[0] = new Node( "5ccf7fc0c75c", 1, 0, 0, 0, 0);  //.147
		NODES[1] = new Node( "5ccf7fc0d218", 1, 0, 0, 0, 1 );  //.179
		NODES[2] = new Node( "5ccf7fc10b08", 1, 0, 0, 0, 2 );  //.169
		NODES[3] = new Node( "5ccf7fc06055", 1, 0, 0, 0, 3 );  //.241
	#if NRNODES>4
		NODES[4] = new Node( "5ccf7fc10aff", 0, 0, 0, 0, 3 );  //.214
	#endif

	#endif
*/
/*

            214
            10'
            ===
241 ------- 25' -------- 147
--
25'
--
179 -------- 25'         169
*/
		double m25 = 7.62/10;
		double m10 = 3.048/10;


		NODES[0] = new Node( "5ccf7fc0c75c", 1, m25, 0, 0, 0 );  //.147
		NODES[1] = new Node( "5ccf7fc0d218", 1, 0, m25, 0, 1 );  //.179
		NODES[2] = new Node( "5ccf7fc10b08", 1, m25, m25, 0, 2 );  //.169
		NODES[3] = new Node( "5ccf7fc06055", 1, 0, 0, 0, 3 );  //.241
		NODES[4] = new Node( "5ccf7fc10aff", 1, m25/2, -m10, 0, 3 );  //.214(213)



		/*
			147 -- 168 in -- 179
			 |
			 |
			162 in
			 |
			 |
			167 ------------ 241
		*/



		ProcessEntries();


//		printf( "Term %d\n", Entries.size() );

		double totalerror = 0.0;
		for( int i = 0; i < NRNODES; i++ )
		{
			for( int j = 0; j < NRNODES; j++ )
			{
				printf( "%d->%d -> %f (%d entries)\n", i, j, NODES[i]->othernodedeltas[j]/NODES[i]->othernodedeltascount[j], NODES[i]->othernodedeltascount[j] );
				double err = NODES[i]->othernodedeltas[j]/NODES[i]->othernodedeltascount[j];
				if( NODES[i]->othernodedeltascount[j] )
					totalerror += fabs( err );
			}
			printf( "\n" );
		}

		if( totalerror < backupError )
		{
			memcpy( BackupToFMatrix, ToFMatrix, sizeof( ToFMatrix ) );
			backupError = totalerror;
		}

		for( int i = 0; i < NRNODES; i++ )
		{
			for( int j = 0; j < NRNODES; j++ )
			{
				printf( "%f ", ToFMatrix[i][j] );
			}
			printf( "\n" );
		}
		fprintf( stderr, "%f Best: %f Anneal: %f\n", totalerror, backupError, anneal );

	}


	return 0;
}




void LoadEntries( const char * filename )
{
	FILE * f = fopen( filename, "r" );
	if( !f )
	{
		fprintf( stderr, "Error: Could not open file \"%s\" for loading\n", filename );
		exit( -5 );
	}

	char *line = NULL;
	size_t len = 0;
	ssize_t read;
	int lineno = 0;



	while ((read = getline(&line, &len, f)) != -1) {
		char ip[32];
		unsigned long long param1;
		char macf[32];
		char mact[32];
		char code[32];
		uint32_t pid;
		uint32_t time;

		lineno++;
		int r = sscanf( line, "%31s %llx %31s %31s %31s %u %u", ip, &param1, macf, mact, code, &pid, &time );
		if( r != 7 )
		{
			fprintf( stderr, "Error reading line %d, got %d params\n", lineno, r );
			continue;
		}

		if( strcmp( code, "ESPEED" ) != 0 )
		{
			//fprintf( stderr, "Error: unmatching code on line %d\n", lineno );
			continue;
		}
		DataEntry e;
		e.IP = ip;
		e.rssi = param1>>40;
		e.macFrom = macf;
		e.macTo = mact;
		e.pid = pid;
		e.time = time;
		Entries.push_back( e );
	}

	fclose( f );
}

int GetNodeId( string mac ) {
	int i; for( i = 0; i < NRNODES; i++ ) if( mac == NODES[i]->myMac ) return i;
	static map< string, int > showns;
	if( showns[mac.c_str()] == 0 )
	{
		fprintf( stderr, "Error: can't find node %s\n", mac.c_str() );
		showns[mac.c_str()] = 1;
	}
	return -1;
}


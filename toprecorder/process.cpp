#include <stdio.h>
#include <vector>
#include <string>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <math.h>

using namespace std;

#define NRNODES 4

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

int still_init = 0;
double MHz = 80; //160;
double SoL = 299792458;
double CalculateToF( string Mac1, string Mac2 ); //In 1/MHz
int GetNodeId( string mac );

class Node
{
public:

	Node() : myMac(""), is_known(false), x(0), y(0), z(0), nodeid(-1), currentpid(-1), VirtualTimeOffset(0), got_time(0), last_time(0), running_time(0) {
		memset( GotPeerTime, 0, sizeof( GotPeerTime ) ); memset( PeerTime, 0, sizeof( PeerTime ) );
	}

	Node( string mac, int ik, double tx, double ty, double tz, int nid ) : myMac( mac ), is_known(ik), x(tx), y(ty), z(tz), nodeid(nodeid), VirtualTimeOffset(0), got_time(0), last_time(0), running_time(0) {
		memset( GotPeerTime, 0, sizeof( GotPeerTime ) ); memset( PeerTime, 0, sizeof( PeerTime ) );
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

	//These three are just for parsing times, nothing tricky.
	double running_time;
	uint32_t last_time;
	bool got_time;


	//Called by transmitters.  Once per
	void SyncTimes( int nodeto )
	{
		int i, j;
		if( is_known )
		{
			const double syncoff = 0.1;
			for( i = 0; i < NRNODES; i++ )
			{
				if( nodeto == i ) continue;
				
				if( GotPeerTime[i] )
				{
					double delta = PeerTime[nodeto] - PeerTime[i];
					LastDelta[i] = delta;
					NODES[i]->LastDelta[nodeto] = delta;
					NODES[nodeto]->LastDelta[i] = delta;
					//fprintf( stderr, "%f %d\n", delta, still_init );
					if( still_init )
					{
						if( delta < -1000000 ) delta = -1000000;
						if( delta > 1000000 ) delta = 1000000;
						delta = delta * syncoff;
					}
					else
					{
						if( delta < -1000 ) delta = -1000;
						if( delta > 1000 ) delta = 1000;
						delta = delta * syncoff;
					}
					NODES[nodeto]->VirtualTimeOffset -= delta;
					NODES[i]->VirtualTimeOffset += delta;
				}
			}
			for( i = 0; i < NRNODES; i++ )
			{
				printf( "%f, %f, %f, %f, %f, ", NODES[i]->VirtualTimeOffset, NODES[i]->LastDelta[0], NODES[i]->LastDelta[1], NODES[i]->LastDelta[2], NODES[i]->LastDelta[3] );
			}
			printf( "%d\n", still_init );
		}
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
		if( is_known )
			PeerTime[nodeto] = RealTime - GetToF( *NODES[nodeto] );
		else
			PeerTime[nodeto] = RealTime;
		SyncTimes( nodeto );
	}

	double GetToF( Node & other )
	{
		if ( !is_known || !other.is_known ) return -1;
		double dx = x - other.x;
		double dy = y - other.y;
		double dz = z - other.z;
		double dist = sqrtf( dx*dx+dy*dy+dz*dz );
		return dist / SoL * (MHz * 1000000); 
	}

	double RxTimeToReal( uint32_t rxtime )  //In ticks.
	{
		if( !got_time )
		{
			got_time = true;
			last_time = rxtime;
			running_time = 0;
			return 0;
		}

		uint32_t rxdtime = rxtime - last_time;
		last_time = rxtime;
		running_time += rxdtime;
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

		still_init = i < 4000; //If still init, update clocks very quickly.
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



	double DistanceMeters162 = 4.1148;
	double DistanceMeters168 = 4.2672;

	NODES[0] = new Node( "5ccf7fc0c75c", 1, 0, 0, 0, 0);  //.147
	NODES[1] = new Node( "5ccf7fc0d218", 1, DistanceMeters168, 0, 0, 1 );  //.179
	NODES[2] = new Node( "5ccf7fc10b08", 1, 0, DistanceMeters162, 0, 2 );  //.169
	NODES[3] = new Node( "5ccf7fc06055", 1, DistanceMeters168, DistanceMeters162, 0, 3 );  //.241

	/*
		147 -- 168 in -- 179
		 |
		 |
		162 in
		 |
		 |
		169 ------------ 241
	*/


	//192.168.11.169 27102e400000 5ccf7fc06055 5ccf7fc10b08 ESPEEDEE 17879 4255185525
	LoadEntries( argv[1] );

	ProcessEntries();

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
			fprintf( stderr, "Error: unmatching code on line %d\n", lineno );
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

int GetNodeId( string mac ) { int i; for( i = 0; i < NRNODES; i++ ) if( mac == NODES[i]->myMac ) return i; fprintf( stderr, "Error: can't find node %s\n", mac.c_str() ); return -1; }


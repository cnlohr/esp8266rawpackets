//Copyright (C) 2015 <>< Charles Lohr, see LICENSE file for more info.
//
//This particular file may be licensed under the MIT/x11, New BSD or ColorChord Licenses.

var mements = 16;


function KickMEMCTRL()
{
	MEMCTRLDataTicker();
}



window.addEventListener("load", KickMEMCTRL, false);

function CommitVal( which )
{
	var addy = parseInt( $("#ent"+which).val() );
	if( addy <= 0 ) return;
	var typ = $("input[name=typ"+which+"]:checked").val();
	if( typ == 'r' )
	{
		QueueOperation( "CR\t" + addy, GotMemR, which );
		console.log( "CR\t" + addy );
	}
	else if( typ == 'o' )
	{
		var cmd =  "CW\t" + addy + "\t" + parseInt( $("#val"+which).val() );
		QueueOperation( cmd,  GotMemW, which );
	}

}

function GotMemR(req,data)
{
	var v = data.split( "\t" );
	$("#val"+req.extra).val( "0x" + parseInt(v[1]).toString(16) );
}

function GotMemW(req,data)
{
}

var storage = "";

function MEMCTRLDataTicker()
{
	if( IsTabOpen('MEMCTRL') ) {
		memplace = -1;
		storage = "";
		QueueOperation( "CD",  GotMEM, -1 );
	}
}

function SaveParams()
{
	var storage = "";
	for( var memplace = 0; memplace < mements; memplace++ )
	{
		var typ = $("input[name=typ" +memplace+ "]:checked").val();
		storage += $("#ent"+memplace).val() + "\t" + typ + "\t";
		storage += $("#val"+memplace).val() + "\n";
	}
	localStorage.memstore = storage;
}

function GotMEM(req,data)
{
	var v = data.split( "\t" );
	if( !IsTabOpen('MEMCTRL') )
	{
		return;
	}
	$("#memplace").val( memplace );

	if( req.extra == -1 )
	{
		$("#memdebug").val( v[1] );
		$("#memdebug2").val( v[2] );
	}
	else
	{
		$("#val"+req.extra).val( "0x" + parseInt(v[1]).toString(16) );
	}

	memplace++;

	if( memplace < mements )
	{
		var typ = $("input[name=typ" +memplace+ "]:checked").val();
		addy = parseInt( $("#ent"+memplace).val() );
		if( typ == 'w' && addy > 0 ) //Watch
		{
			QueueOperation( "CR\t" + addy, GotMEM, memplace );			
		}
		else
		{
			QueueOperation( "CD",  GotMEM, -1 );
		}
	}
	else
	{
		memplace = -1;
		SaveParams();
		QueueOperation( "CD",  GotMEM, -1 );
	}

	//console.log( data );
}


$(document).ready(function () {
	var dasent = "<TABLE WIDTH=100><TR><TH>address</TH><TH>None</TH><TH>Input</TH><TH>Watch</TH><TH>Output</TH><TH>Value</TH><TH>Submit</TH></TR>";
	for( var i = 0; i < mements; i++ )
	{
		let iv = i;
		dasent += "<tr><td><input type=text id=ent"+iv+"></td><td><input type=radio name=\"typ"+iv+"\" value=\"n\">None</td><td><input type=radio name=\"typ"+iv+"\" value=\"i\">Input</td><td><input type=radio name=\"typ"+iv+"\" value=\"w\">Watch</td><td><input type=radio name=\"typ"+iv+"\" value=\"o\">Output</td><td><input type=text id=val"+iv+"></td><td><input type=submit onclick=\"CommitVal("+iv+");\" value=\"Go\"></TD></td></tr>";
	}
	$("#controllist").html( dasent + "</table>" );

	if( localStorage.memstore )
	{
		var lines = localStorage.memstore.split( "\n" );
		for( var i = 0; i < lines.length; i++ )
		{
			var sp = lines[i].split( "\t" );
			if( sp.length < 3 ) continue;
			$("#ent"+i).val(sp[0]);
			if( sp[1] == 'n' ) $("input[name=typ" +i+ "]")[0].checked = true;
			if( sp[1] == 'i' ) $("input[name=typ" +i+ "]")[1].checked = true;
			if( sp[1] == 'w' ) $("input[name=typ" +i+ "]")[2].checked = true;
			if( sp[1] == 'o' ) $("input[name=typ" +i+ "]")[3].checked = true;
			$("#val"+i).val(sp[2]);
		}
	}
});



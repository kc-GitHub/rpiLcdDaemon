=head1
	00_rpiLcd.pm

=head1 SYNOPSIS
	...
	contributed by Dirk Hoffmann 10/2012 - 2013
	$Id$

=head1 DESCRIPTION
	...

=head1 AUTHOR - Dirk Hoffmann
	dirk@FHEM_Forum (forum.fhem.de)
=cut
 
package main;

use strict;
use warnings;
use Data::Dumper;

use vars qw {%attr %defs %selectlist}; #supress errors in Eclipse EPIC

# FHEM Inteface related functions
sub rpiLCD_Initialize($);
sub rpiLCD_Define($$);
sub rpiLCD_Ready($);
sub rpiLCD_DoInit($);
sub rpiLCD_Undef($$);
sub rpiLCD_Read($);
sub rpiLCD_Set($@);

my %sets = (
	'text' => 1,
	'cls' => 1,
);

my %menuItemns = (
	0 => 'Menue 0                     ',
	1 => 'Menue 1                     ',
	2 => 'Menue 2                     ',
	3 => 'Menue 3                     ',
	4 => 'Menue 4                     ',
	5 => 'Menue 5                     ',
	6 => 'Menue 6                     ',
	7 => 'Menue 7                     ',
);

my $maxMenuLines = 5;
my $menuPos = 0;

my $menuOn = 0;

=head2 rpiLCD_Initialize
	Title		: rpiLCD_Initialize
	Usage		: rpiLCD_Initialize(\%hash);
	Function	: Implements Initialize function
	Returns 	: nothing
	Args 		: named arguments:
				: -argument1 => hash	: hash of device addressed
=cut
sub rpiLCD_Initialize($) {
	my ($hash) = @_;

	require $attr{global}{modpath} . '/FHEM/DevIo.pm';

	$hash->{DefFn}		= 'rpiLCD_Define';
	$hash->{ReadyFn}	= 'rpiLCD_Ready';
	$hash->{UndefFn}	= 'rpiLCD_Undef';
	$hash->{AttrList}	= 'do_not_notify:0,1 dummy:1,0 showtime:1,0 '.
						  'loglevel:0,1,2,3,4,5,6';

	# Provider
	$hash->{ReadFn}		= 'rpiLCD_Read';
	$hash->{SetFn}		= 'rpiLCD_Set';
}

=head2 rpiLCD_Define
	Title		: rpiLCD_Define
	Function	: Implements DefFn function
	Returns 	: string | undef
	Args 		: named arguments:
				: -argument1 => hash	: hash of device addressed
				: -argument2 => string	: definition string
=cut
sub rpiLCD_Define($$) {
	my ($hash, $def) = @_;
	my @a = split('[ \t][ \t]*', $def);

	if( (@a < 3)) {
		my $msg = 'wrong syntax: define <name> rpiLcd {none | hostname:port}';
		Log (1, $msg);
		return $msg;
	}

	DevIo_CloseDev($hash);

	my $name = $a[0];
	my $dev = $a[2];

	$dev .= ":1234" if($dev !~ m/:/ && $dev ne "none" && $dev !~ m/\@/);

	if($dev eq 'none') {
		Log (1, 'rpiLcd device is none, commands will be echoed only');
		$attr{$name}{dummy} = 1;
		delete($selectlist{$name . '.' . $hash->{DEF}});
		return undef;
	}

	$hash->{DeviceName} = $dev;
	my $ret = DevIo_OpenDev($hash, 0, 'rpiLCD_DoInit');

	return $ret;
}

=head2 rpiLCD_Ready
	Title:		rpiLCD_Ready
	Function:	Implements ReadyFn function.
	Returns:	boolean
	Args:		named arguments:
				-argument1 => hash:		$hash		hash of device
=cut
sub rpiLCD_Ready($) {
	my ($hash) = @_;

	return DevIo_OpenDev($hash, 1, 'rpiLCD_DoInit') if($hash->{STATE} eq 'disconnected');

	# This is relevant for windows/USB only
	my $po = $hash->{USBDev};
	my ($BlockingFlags, $InBytes, $OutBytes, $ErrorFlags) = $po->status;
	return ($InBytes>0);
}

=head2 rpiLCD_DoInit
	Title		: rpiLCD_DoInit
	Function	: Implements DoInit function. Initialize the serial device
	Returns 	: string | undef
	Args 		: named arguments:
				: -argument1 => hash	: hash of device addressed
=cut
sub rpiLCD_DoInit($) {
	my ($hash) = @_;
	my $name = $hash->{NAME};

	lcd_init($hash);
	
	return undef;
}

=head2 rpiLCD_Undef
	Title:		rpiLCD_Undef
	Function:	Implements UndefFn function.
	Returns:	string|undef
	Args:		named arguments:
				-argument1 => hash:		$hash	hash of device addressed
				-argument1 => string:	$name	name of device
=cut
sub rpiLCD_Undef($$) {
	my ($hash, $name) = @_;

	foreach my $d (sort keys %defs) {
		if(defined($defs{$d}) && defined($defs{$d}{IODev}) && $defs{$d}{IODev} == $hash) {
			Log (GetLogLevel($name,4), 'deleting port for ' . $d);
			delete $defs{$d}{IODev};
		}
	}

	DevIo_CloseDev($hash);
	return undef;
}

=head2 rpiLCD_Read
	Title:		rpiLCD_Read
	Function:	Implements ReadFn function.
				called from the global loop, when the select for hash->{FD} reports data
	Returns:	nothing
	Args:		named arguments:
				-argument1 => hash	$hash	hash of device
=cut
sub rpiLCD_Read($) {
	my ($hash) = @_;

	my $buffer = DevIo_SimpleRead($hash);
	
	if (defined($buffer)) {
		lcd_parseCommand($hash, $buffer);
	}
}

=head2 rpiLCD_Set
	Title:		rpiLCD_Set
	Function:	Implements SetFn function.
	Returns:	string
	Args:		named arguments:
				-argument1 => hash:		$hash		hash of device
				-argument1 => array:	@a		argument array
=cut
sub rpiLCD_Set($@) {
	my ($hash, @a) = @_;

	my $name =$a[0];
	my $cmd = $a[1];
	my $msg = '';
	
	return '"set HM485" needs one or more parameter' if(@a < 2);
	
	if(!defined($sets{$cmd})) {
		return 'Unknown argument ' . $cmd . ', choose one of ' . join(' ', keys %sets)
	}

	if ($cmd eq 'raw') {
		my $paramError = 0;
		if (@a == 6 || @a == 7) {
			if (($a[2] ne 'FE' && $a[2] ne 'FD' && $a[2] !~ m/^[A-F0-9]{8}$/i ) ||
				 $a[3] !~ m/^[A-F0-9]{8}$/i || $a[4] !~ m/^[A-F0-9]{2}$/i ||
				 $a[5] !~ m/^[A-F0-9]{8}$/i || $a[6] !~ m/^[A-F0-9]{1,251}$/i ) {
					
					$paramError = 1
			}
		} else {
			$paramError = 1;
		}

		return	'"set HM485 raw" needs 5 or 6 parameter Sample: [SS] TTTTTTTT CC SSSSSSSS D...' . "\n" .
				'Set sender address to 00000000 to use address from configuration.' . "\n\n" . 
				'[SS]: optional Startbyte (FD or FE),' . "\n" .
				'   T: 8 byte target address, C: Control byte, S: 8 byte sender address, D: data bytes' . "\n"
				if ($paramError);

		FHEM::HM485::Communication::sendRawQueue(
			$hash, pack('H*', $a[3]), hex($a[4]), pack('H*', $a[5]), pack('H*', $a[6])
		);

	} elsif ($cmd eq 'discovery') {
		if (FHEM::HM485::Util::checkForAutocreate()) {
			# TODO: set timeout from outer
			my $timeout = 30;
	
			$msg = FHEM::HM485::Communication::cmdDiscovery($hash, $timeout);
		} else {
			$msg = 'Please activate and enable autocreate first.'
		}

	} elsif ($cmd eq 'test') {
		# TODO: delete later
		my $senderAddr = pack ('H*', AttrVal($hash->{NAME}, 'hmwId', '00000001'));

		FHEM::HM485::Communication::sendRawQueue($hash, pack ('H*', 'FFFFFFFF'), 0x9C, $senderAddr, chr(0x31));
		FHEM::HM485::Communication::sendRawQueue($hash, pack ('H*', 'FFFFFFFF'), 0x9C, $senderAddr, chr(0x32));
		FHEM::HM485::Communication::sendRawQueue($hash, pack ('H*', 'FFFFFFFF'), 0x9C, $senderAddr, chr(0x33));
		FHEM::HM485::Communication::sendRawQueue($hash, pack ('H*', 'FFFFFFFF'), 0x9C, $senderAddr, chr(0x34));
		FHEM::HM485::Communication::sendRawQueue($hash, pack ('H*', 'FFFFFFFF'), 0x9C, $senderAddr, chr(0x35));
		FHEM::HM485::Communication::sendRawQueue($hash, pack ('H*', 'FFFFFFFF'), 0x9C, $senderAddr, chr(0x36));
		FHEM::HM485::Communication::sendRawQueue($hash, pack ('H*', 'FFFFFFFF'), 0x9C, $senderAddr, chr(0x37));
		FHEM::HM485::Communication::sendRawQueue($hash, pack ('H*', 'FFFFFFFF'), 0x9C, $senderAddr, chr(0x38));
		FHEM::HM485::Communication::sendRawQueue($hash, pack ('H*', 'FFFFFFFF'), 0x9C, $senderAddr, chr(0x39));
		FHEM::HM485::Communication::sendRawQueue($hash, pack ('H*', 'FFFFFFFF'), 0x9C, $senderAddr, chr(0x3A));
		FHEM::HM485::Communication::sendRawQueue($hash, pack ('H*', 'FFFFFFFF'), 0x9C, $senderAddr, chr(0x3B));
		FHEM::HM485::Communication::sendRawQueue($hash, pack ('H*', 'FFFFFFFF'), 0x9C, $senderAddr, chr(0x3C));
		FHEM::HM485::Communication::sendRawQueue($hash, pack ('H*', 'FFFFFFFF'), 0x9C, $senderAddr, chr(0x3E));
		FHEM::HM485::Communication::sendRawQueue($hash, pack ('H*', 'FFFFFFFF'), 0x9C, $senderAddr, chr(0x3F));
		FHEM::HM485::Communication::sendRawQueue($hash, pack ('H*', 'FFFFFFFF'), 0x9C, $senderAddr, chr(0x40));
		FHEM::HM485::Communication::sendRawQueue($hash, pack ('H*', 'FFFFFFFF'), 0x9C, $senderAddr, chr(0x41));
		FHEM::HM485::Communication::sendRawQueue($hash, pack ('H*', 'FFFFFFFF'), 0x9C, $senderAddr, chr(0x42));
		FHEM::HM485::Communication::sendRawQueue($hash, pack ('H*', 'FFFFFFFF'), 0x9C, $senderAddr, chr(0x43));
		FHEM::HM485::Communication::sendRawQueue($hash, pack ('H*', 'FFFFFFFF'), 0x9C, $senderAddr, chr(0x44));
		FHEM::HM485::Communication::sendRawQueue($hash, pack ('H*', 'FFFFFFFF'), 0x9C, $senderAddr, chr(0x45));
		FHEM::HM485::Communication::sendRawQueue($hash, pack ('H*', 'FFFFFFFF'), 0x9C, $senderAddr, chr(0x46));
		FHEM::HM485::Communication::sendRawQueue($hash, pack ('H*', 'FFFFFFFF'), 0x9C, $senderAddr, chr(0x47));
		
		$msg = 'Test done';
	}

	return $msg;
}

sub rpiLCD_command(@) {
	my ($hash, $msg, $nonl) = @_;

	return if(!$hash || AttrVal($hash->{NAME}, "dummy", undef));
	my $name = $hash->{NAME};
	my $ll5 = GetLogLevel($name,5);

	Log (1, 'rpiLcd command: '.$msg);

	$msg .= "\r\n\r\n";

	syswrite($hash->{TCPDev}, $msg) if($hash->{TCPDev});
}



###############################################################################

sub lcd_parseCommand() {
	my ($hash, $command) = @_;
	
	if($command =~ m/ButtonStatus:/) {
		my $name = $hash->{NAME};
		
		my ($txtBtnStatus, $txtBtnCounter) = split(", ", $command);
		my (undef, $btnStatus) = split(": ", $txtBtnStatus);
		my (undef, $btnCounter) = split(": ", $txtBtnCounter);

		$btnStatus = 0 if (!defined($btnStatus));
		
		processButtons($hash, $btnStatus);
		
		Log (1, 'Button pressed: ---------------------> ' . $btnStatus);
		
		my %readings = (
			'BTN_LEFT'   => 'none',
			'BTN_CENTER' => 'none',
			'BTN_RIGHT'  => 'none'
		);

		$readings{'BTN_LEFT'} = 'Pressed' if ($btnStatus & 0b1);
		$readings{'BTN_LEFT'}.= '_long'   if ($btnStatus & 0b10);

		$readings{'BTN_CENTER'} = 'Pressed' if ($btnStatus &  0b100);
		$readings{'BTN_CENTER'}.= '_long'   if ($btnStatus & 0b1000);

		$readings{'BTN_RIGHT'} = 'Pressed' if ($btnStatus &  0b10000);
		$readings{'BTN_RIGHT'}.= '_long'   if ($btnStatus & 0b100000);
		
		my $timeNow = TimeNow();
		foreach my $r (keys %readings) {
			$hash->{READINGS}{$r}{TIME} = $timeNow;
			$hash->{READINGS}{$r}{VAL} = $readings{$r};
		}

		my @btnStates;
		push(@btnStates, 'BTN_LEFT: '   . $readings{'BTN_LEFT'})   if ($readings{'BTN_LEFT'} ne 'none');
		push(@btnStates, 'BTN_CENTER: ' . $readings{'BTN_CENTER'}) if ($readings{'BTN_CENTER'} ne 'none');
		push(@btnStates, 'BTN_RIGHT: '  . $readings{'BTN_RIGHT'})  if ($readings{'BTN_RIGHT'} ne 'none');

		my $state = join(' ', @btnStates);
		$state = ($state ne '') ? $state : 'none';

		$hash->{CHANGED}[0] = $state;
		$hash->{STATE} = $state;
		$hash->{READINGS}{STATE}{VAL} = $state;
		$hash->{READINGS}{STATE}{TIME} = $timeNow;

		DoTrigger($hash->{NAME}, undef);		
	}
	
}

sub processButtons() {
	my ($hash, $buttonState) = @_;

	if ( ($buttonState & 0b1000) > 0 && $menuOn < 1) {
		$menuOn = 1;
		$menuPos = 0;
		
		lcd_contentClear($hash);
		
		refreshMenu($hash);

	} elsif ( ($buttonState & 0b1000) > 0 && $menuOn == 1) {
		$menuOn = 0;
		lcd_contentDefault($hash);
	}
	
	if ($menuOn) {
		if ( ($buttonState & 0b10000) > 0) {
			$menuPos++
		}

		if ( ($buttonState & 0b1) > 0) {
			$menuPos--
		}

		refreshMenu($hash);
	}
}

sub refreshMenu() {
	my ($hash) = @_;
	
	my $maxMenuItems = scalar(keys %menuItemns);
	
	$menuPos = ($menuPos >=0) ? $menuPos : 0;
	$menuPos = ($menuPos <$maxMenuItems) ? $menuPos : $maxMenuItems-1;
	
	my $ml = $menuPos;
	my $menuLineStart = 0;
	if ($menuPos > ($maxMenuLines-1)) {
		$menuLineStart = $menuPos - $maxMenuLines+1;
		$ml = $maxMenuLines-1;
	}
	
	my $line = 13;
	for my $i ($menuLineStart..($maxMenuLines+$menuLineStart)){
		my $invert = ($i == $menuPos) ? 1 : 0;
		lcd_text($hash, 0, $line, $menuItemns{$i}, $invert);

		$line = $line + 10;
	}
}

sub lcd_init() {
	my ($hash) = @_;
	
	rpiLCD_command($hash, "cls");

	lcd_setHeader($hash);
	lcd_contentDefault($hash);
}

sub lcd_setHeader() {
	my ($hash) = @_;
	
	rpiLCD_command($hash, "date,18,-3,1,0");
	rpiLCD_command($hash, "time,88,-3,1,0");

	rpiLCD_command($hash, "rect,2,4,12,8,1");
	rpiLCD_command($hash, "line,0,4,7,0");
	rpiLCD_command($hash, "line,7,0,14,4");
	rpiLCD_command($hash, "line,0,4,14,4");

	rpiLCD_command($hash, "line,0,11,128,11");
}

sub lcd_contentDefault() {
	my ($hash) = @_;
	
	lcd_contentClear($hash);
	rpiLCD_command($hash, "bmp,0,14,/opt/rpiLcdDaemon/images/fhem2.bmp");
	rpiLCD_command($hash, "bmp,66,15,/opt/rpiLcdDaemon/images/fhem3.bmp");

	rpiLCD_command($hash, "setFont,0");
	rpiLCD_command($hash, "text,76,34,Home,0,1");
	rpiLCD_command($hash, "text,60,44,Automation,0,1");
	rpiLCD_command($hash, "text,70,54,Server,0,1");
}

sub lcd_contentClear() {
	my ($hash) = @_;

	rpiLCD_command($hash, "rect,0,12,128,64,0");
}

sub lcd_text() {
	my ($hash, $x, $y, $text, $invert) = @_;

	rpiLCD_command($hash, 'text,' . $x . ',' . $y . ',' . $text . ',' . $invert . ',1');
}


1;

=pod
=begin html

<a name="HM485"></a>
	<h3>HM485</h3>
	<p> FHEM module to commmunicate with HM485 devices</p>
	<ul>
		<li>...</li>
		<li>...</li>
		<li>...</li>
	</ul>

=end html
=cut

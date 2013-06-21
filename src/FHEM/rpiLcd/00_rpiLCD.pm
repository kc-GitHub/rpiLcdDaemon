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

sub lcd_init($);
sub lcd_parseCommand($$);
sub processButtons($$);
sub lcd_contentDefault($);
sub lcd_setHeader($);
sub refreshMenu($);
sub lcd_text($$$$$);

sub lcd_contentClear($);

my %sets = (
	'text' => ' ',
	'cls' => ' ',
	'led' => ' ',
	'backlight' => '1,2,3,4,5,10,15,20,25,30,35,40,45,50,60,70,80,90,100,110,125,150,200,225,254',
);

my %menuItemns = (
	1 => {
		item => 'IP-Adresse(n) zeigen ',
		func => 'menuDisplayIP'
	},
	2 => {
		item => 'Menue 2              ',
		func => ''
	},
	3 => {
		item => 'Menue 3              ',
		func => ''
	},
	4 => {
		item => 'Menue 4              ',
		func => ''
	},
	5 => {
		item => 'Ausschalten          ',
		func => 'menuShutdown'
	},
);

my $maxMenuLines = 5;
my $menuItemSelected = 0;
my $menuItemActive = 0;

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
	$hash->{DeleteFn}	= 'rpiLCD_Delete';
	$hash->{ShutdownFn}	= 'rpiLCD_Shutdown';
	
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

	rpiLCD_modifyJsInclude();

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

	DevIo_CloseDev($hash);
	return undef;
}

=head2 rpiLCD_Delete
	Title:		rpiLCD_Delete
	Function:	Implements DeleteFn function.
	Returns:	string|undef
	Args:		named arguments:
				-argument1 => hash:		$hash	hash of device addressed
				-argument1 => string:	$name	name of device
=cut
sub rpiLCD_Delete($$) {
	my ($hash, $name) = @_;
	Log (1, "Delete");

	lcd_contentClear($hash);
	rpiLCD_command($hash, "bmp,0,14,/opt/rpiLcdDaemon/images/fhem2.bmp");
	rpiLCD_command($hash, "bmp,66,15,/opt/rpiLcdDaemon/images/fhem3.bmp");

	rpiLCD_command($hash, "setFont,1");
	rpiLCD_command($hash, "text,76,34,Stopped!,0,1");

	return undef;
}

=head2 rpiLCD_Shutdown
	Title:		rpiLCD_Delete
	Function:	Implements DeleteFn function.
	Returns:	void
	Args:		named arguments:
				-argument1 => hash:		$hash	hash of device addressed
=cut
sub rpiLCD_Shutdown($$) {
	my ($hash) = @_;
	Log (1, "rpiLCD_Shutdown");

	lcd_contentClear($hash);
	rpiLCD_command($hash, "bmp,0,14,/opt/rpiLcdDaemon/images/fhem2.bmp");
	rpiLCD_command($hash, "bmp,66,15,/opt/rpiLcdDaemon/images/fhem3.bmp");

	rpiLCD_command($hash, "setFont,1");
	rpiLCD_command($hash, "text,55,35,Stopped!,0,1");
}

sub rpiLCD_modifyJsInclude() {
	my $vars = '';
	$data{FWEXT}{'/fhem/pgm2/'}{SCRIPT} = 'station-clock.js"></script>' .
									 '<script type="text/javascript" src="/fhem/js/excanvas.js"></script>' .
									 '<script type="text/javascript" src="/fhem/js/user.js"></script>' .
									 '<script type="text/javascript" charset="UTF-8';
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
	
	return '"set rpiLCD" needs one or more parameter' if(@a < 2);
	
	if(!defined($sets{$cmd})) {
		
		my $cmdList = '';
		foreach my $key (sort keys %sets) {
			Log (1, $key);
			if ($sets{$key} && $sets{$key} ne ' ') {
				$cmdList.= $key . ':' . $sets{$key} . ' ';
			} else {
				$cmdList.= $key . ' ';
			}
		}
		
		return 'Unknown argument ' . $cmd . ', choose one of ' . $cmdList;
	}

	if ($cmd eq 'cls') {
		lcd_contentClear($hash);

	} elsif ($cmd eq 'led') {
		my $led = $a[2];
		my $value = $a[3];
		rpiLCD_command($hash, 'setLed,' . $led . ',' . $value);

	} elsif ($cmd eq 'backlight') {
		if ($a[2]) {
			my $backlight = ($a[2] > 1) ? $a[2] : 1;
			$backlight = ($backlight < 255) ? $backlight : 254;
			rpiLCD_command($hash, 'setBacklight,' . $backlight);
		} else {
			return "Unknown argument $a[2], choose one of 1:2:3:4";
		}
	}

	return $msg;
}

sub rpiLCD_command(@) {
	my ($hash, $msg, $nonl) = @_;

	return if(!$hash || AttrVal($hash->{NAME}, "dummy", undef));
	my $name = $hash->{NAME};
	my $ll5 = GetLogLevel($name,5);

#	Log (1, 'rpiLcd command: '.$msg);

	$msg .= "\r\n";
	syswrite($hash->{TCPDev}, $msg) if($hash->{TCPDev});
}



###############################################################################

sub lcd_parseCommand($$) {
	my ($hash, $command) = @_;
	
	if($command =~ m/ButtonStatus:/) {
		my $name = $hash->{NAME};
		
		my ($txtBtnStatus, $txtBtnCounter) = split(", ", $command);
		my (undef, $btnStatus) = split(": ", $txtBtnStatus);
		my (undef, $btnCounter) = split(": ", $txtBtnCounter);

		$btnStatus = 0 if (!defined($btnStatus));
		
		processButtons($hash, $btnStatus);
		
#		Log (1, 'Button pressed: ---------------------> ' . $btnStatus);
		
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

sub processButtons($$) {
	my ($hash, $buttonState) = @_;

	if ($menuItemActive > 0) {
		if ( ($buttonState & 0b1000) > 0) {							# End Menu Item (long middle press)
			$menuItemSelected = 0;
			$menuItemActive = 0;
			$menuOn = 0;
			lcd_contentDefault($hash);
		} else {
			Log (1, "Select menuItem: $menuItemSelected, buttonState: $buttonState");
			my $func = $menuItemns{$menuItemSelected}{func};
			if (defined ($func) && $func) {
				no strict "refs";
				my $ret = &{$func}($hash, $buttonState);
				use strict "refs";
			}
		}

	} else {

		if ( ($buttonState & 0b1000) > 0 && $menuOn < 1) {			# Activate menu (long middle press)
			$menuOn = 1;
			$menuItemSelected = 1;
			
			lcd_contentClear($hash);
			
			refreshMenu($hash);
	
		} elsif ( ($buttonState & 0b1000) > 0 && $menuOn == 1) {	# Deactivate menu (long middle press)
			$menuOn = 0;
			$menuItemSelected = 0;
			lcd_contentDefault($hash);

		} elsif (($buttonState & 0b100) > 0) {							# Select Menu Item (short middle press)
			$menuItemActive = $menuItemSelected;
		}
		
		if ($menuOn && $menuItemActive == 0) {
			if ( ($buttonState & 0b10000) > 0) {					# Up (activated Menu)
				$menuItemSelected++
			}
	
			if ( ($buttonState & 0b1) > 0) {						# Down (activated Menu)
				$menuItemSelected--
			}
	
			refreshMenu($hash);
		}
	}

}

sub refreshMenu($) {
	my ($hash) = @_;
	
	my $maxMenuItems = scalar(keys %menuItemns);
	
	Log(1, $menuItemSelected . "-" . $maxMenuItems . "-" . scalar(keys %menuItemns));

	$menuItemSelected = ($menuItemSelected >=1) ? $menuItemSelected : 1;
	$menuItemSelected = ($menuItemSelected <= $maxMenuItems) ? $menuItemSelected : $maxMenuItems;
	
	
	my $ml = $menuItemSelected;
	my $menuLineStart = 1;
	if ($menuItemSelected > ($maxMenuLines)) {
		$menuLineStart = $menuItemSelected - $maxMenuLines+1;
		$ml = $maxMenuLines;
	}
	
	my $line = 13;
	for my $i ($menuLineStart..($maxMenuLines+$menuLineStart-1)){
		my $invert = ($i == $menuItemSelected) ? 1 : 0;
		lcd_text($hash, 0, $line, $menuItemns{$i}{item}, $invert);

		$line = $line + 10;
	}
}

sub lcd_init($) {
	my ($hash) = @_;
	
	rpiLCD_command($hash, "cls");
	rpiLCD_command($hash, "setBacklight,30");

	lcd_setHeader($hash);
	lcd_contentDefault($hash);
}

sub lcd_setHeader($) {
	my ($hash) = @_;
	
	rpiLCD_command($hash, "date,18,-3,1,0");
	rpiLCD_command($hash, "time,88,-3,1,0");

	rpiLCD_command($hash, "rect,2,4,12,8,1");
	rpiLCD_command($hash, "line,0,4,7,0");
	rpiLCD_command($hash, "line,7,0,14,4");
	rpiLCD_command($hash, "line,0,4,14,4");

	rpiLCD_command($hash, "line,0,11,128,11");
}

sub lcd_contentDefault($) {
	my ($hash) = @_;
	
	lcd_contentClear($hash);
	rpiLCD_command($hash, "bmp,0,14,/opt/rpiLcdDaemon/images/fhem2.bmp");
	rpiLCD_command($hash, "bmp,66,15,/opt/rpiLcdDaemon/images/fhem3.bmp");

	rpiLCD_command($hash, "setFont,0");
	rpiLCD_command($hash, "text,76,34,Home,0,1");
	rpiLCD_command($hash, "text,60,44,Automation,0,1");
	rpiLCD_command($hash, "text,70,54,Server,0,1");
}

sub lcd_contentClear($) {
	my ($hash) = @_;

	rpiLCD_command($hash, "rect,0,12,128,64,0");
}

sub lcd_text($$$$$) {
	my ($hash, $x, $y, $text, $invert) = @_;

	rpiLCD_command($hash, 'text,' . $x . ',' . $y . ',' . $text . ',' . $invert . ',1');
}

sub menuDisplayIP() {
	my ($hash, $buttonState) = @_;

	lcd_contentClear($hash);
	rpiLCD_command($hash, 'setFont,0');
	rpiLCD_command($hash, 'text,0,13,IP-Adresse(n):,0,1');
	
	my $cmdLine = 'ip -o addr show | awk \'/inet/ {print $2, $3, $4}\'';
	my @ips = `$cmdLine`;
	my $line = 23;
	foreach my $ipLine (@ips) {
		my ($interface, undef, $ipParts) = split(' ', $ipLine);
		my ($ip) = split('/', $ipParts);
		if ($interface ne 'lo') {
			rpiLCD_command($hash, 'text,0,' . $line . ',' . $interface . ':' . $ip . ',0,1');
			$line = $line +10;
		}
						
		Log (1, "IP: $interface: $ip");
	}
}

sub menuShutdown() {
	my ($hash, $buttonState) = @_;

	lcd_contentClear($hash);
	rpiLCD_command($hash, 'setFont,1');
	rpiLCD_command($hash, 'text,23,25,Shutdown,0,1');
	
	`sudo /sbin/halt`;
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

=head1
	00_rpiLcd.pm

=head1 SYNOPSIS
	...
	contributed by Dirk Hoffmann 10/2012 - 2014
	$Id$

=head1 DESCRIPTION

	todo's
	- implement path variable to images
	- transfer menu definition to external module and use it with 'use <module>'
	- to separate between LCD functions and user alterable functions

=head1 AUTHOR - Dirk Hoffmann
	dirk@FHEM_Forum (forum.fhem.de)
=cut
 
package main;

use strict;
use warnings;

use Data::Dumper;	# for debugging purpose only

use vars qw {%attr %defs %selectlist}; #supress errors in Eclipse EPIC

use constant {
	RPI_LCD_Version => 0.6
};

# FHEM Interface related functions
sub RPI_LCD_Initialize($);
sub RPI_LCD_Define($$);
sub RPI_LCD_Ready($);
sub RPI_LCD_DoInit($);
sub RPI_LCD_Undef($$);
sub RPI_LCD_Read($);
sub RPI_LCD_Set($@);

sub RPI_LCD_init($);
sub RPI_LCD_parseCommand($$);
sub RPI_LCD_command(@);
sub RPI_LCD_contentDefault($);
sub RPI_LCD_clearContent($);
sub RPI_LCD_clearAll($);
sub RPI_LCD_setHeader($);
sub RPI_LCD_text($$$$$;$);
sub RPI_LCD_bmp($$$$);
sub RPI_LCD_line($$$$$);
sub RPI_LCD_rect($$$$$;$);
sub RPI_LCD_circle($$$$);
sub RPI_LCD_setFont($$);
sub RPI_LCD_date($$$$);
sub RPI_LCD_time($$$$$);
sub RPI_LCD_stopDate($);
sub RPI_LCD_stopTime($);

sub RPI_LCD_processButtons($$);
sub RPI_LCD_refreshMenu($);

my %sets = (
	'rect' => ' ',
	'line' => ' ',
	'text' => ' ',
	'bmp' => ' ',
	'setFont' => ' ',
	'date' => ' ',
	'time' => ' ',
	'stopTime' => ' ',
	'stopDate' => ' ',
	'circle' => ' ',
	'cls' => ' ',
	'clearAll' => ' ',
	'led' => ' ',
	'backlight' => '0,1,2,3,4,5,10,15,20,25,30,35,40,45,50,60,70,80,90,100,110,125,150,200,225,255',
);

my %menuItemns = (
	1 => {
		item => 'IP-Adresse(n) zeigen ',
		func => 'RPI_LCD_menuDisplayIP'
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
		func => 'RPI_LCD_menuShutdown'
	},
);

my $maxMenuLines = 5;
my $menuItemSelected = 0;
my $menuItemActive = 0;

my $menuOn = 0;

=head2 RPI_LCD_Initialize
	Title:    RPI_LCD_Initialize
	Function: Implements Initialize function
	Returns:  nothing
	Args:     named arguments:
	          -argument1 => hash: hash of device addressed
=cut
sub RPI_LCD_Initialize($) {
	my ($hash) = @_;

	require $attr{global}{modpath} . '/FHEM/DevIo.pm';

	$hash->{DefFn}      = 'RPI_LCD_Define';
	$hash->{ReadyFn}    = 'RPI_LCD_Ready';
	$hash->{UndefFn}    = 'RPI_LCD_Undef';
	$hash->{DeleteFn}   = 'RPI_LCD_Delete';
	$hash->{ShutdownFn} = 'RPI_LCD_Shutdown';
	
	$hash->{AttrList} = 'do_not_notify:0,1 dummy:1,0 showtime:1,0 '.
	                    'loglevel:0,1,2,3,4,5,6';

	# Provider
	$hash->{ReadFn}     = 'RPI_LCD_Read';
	$hash->{SetFn}      = 'RPI_LCD_Set';
}

=head2 RPI_LCD_Define
	Title:    RPI_LCD_Define
	Function: Implements DefFn function
	Returns:  string | undef
	Args:     named arguments:
	          -argument1 => hash: hash of device addressed
	          -argument2 => string: definition string
=cut
sub RPI_LCD_Define($$) {
	my ($hash, $def) = @_;
	my @a = split('[ \t][ \t]*', $def);

	if( (@a < 3)) {
		my $msg = 'wrong syntax: define <name> RPI_LCD {none | hostname:port}';
		Log (1, $msg);
		return $msg;
	}

	DevIo_CloseDev($hash);

	my $name = $a[0];
	my $dev = $a[2];

	$dev .= ":1234" if($dev !~ m/:/ && $dev ne "none" && $dev !~ m/\@/);

	if($dev eq 'none') {
		Log (1, 'RPI_LCD device is none, commands will be echoed only');
		$attr{$name}{dummy} = 1;
		delete($selectlist{$name . '.' . $hash->{DEF}});
		return undef;
	}

	$hash->{DeviceName} = $dev;
	$hash->{VERSION} = RPI_LCD_Version;
	my $ret = DevIo_OpenDev($hash, 0, 'RPI_LCD_DoInit');

	return $ret;
}

=head2 RPI_LCD_Ready
	Title:    RPI_LCD_Ready
	Function: Implements ReadyFn function.
	Returns:  boolean
	Args:     named arguments:
	          -argument1 => hash: $hash hash of device
=cut
sub RPI_LCD_Ready($) {
	my ($hash) = @_;

	return DevIo_OpenDev($hash, 1, 'RPI_LCD_DoInit') if($hash->{STATE} eq 'disconnected');

	# This is relevant for windows/USB only
	my $po = $hash->{USBDev};
	my ($BlockingFlags, $InBytes, $OutBytes, $ErrorFlags) = $po->status;

	return ($InBytes>0);
}

=head2 RPI_LCD_DoInit
	Title:    RPI_LCD_DoInit
	Function: Implements DoInit function. Initialize the serial device.
	Returns:  string | undef
	Args:     named arguments:
	          -argument1 => hash: hash of device addressed
=cut
sub RPI_LCD_DoInit($) {
	my ($hash) = @_;
	my $name = $hash->{NAME};

	RPI_LCD_init($hash);

	return undef;
}

=head2 RPI_LCD_Undef
	Title:    RPI_LCD_Undef
	Function: Implements UndefFn function.
	Returns:  string|undef
	Args:     named arguments:
	          -argument1 => hash: $hash hash of device addressed
	          -argument2 => string: $name name of device
=cut
sub RPI_LCD_Undef($$) {
	my ($hash, $name) = @_;

	DevIo_CloseDev($hash);
	return undef;
}

=head2 RPI_LCD_Delete
	Title:    RPI_LCD_Delete
	Function: Implements DeleteFn function.
	Returns:  string|undef
	Args:     named arguments:
	          -argument1 => hash: $hash hash of device addressed
	          -argument2 => string: $name name of device
=cut
sub RPI_LCD_Delete($$) {
	my ($hash, $name) = @_;
	Log (1, "Delete");

	RPI_LCD_clearContent($hash);
	RPI_LCD_bmp($hash, 0, 14, '/opt/rpiLcdDaemon/images/fhem2.bmp');
	RPI_LCD_bmp($hash, 66, 15, '/opt/rpiLcdDaemon/images/fhem3.bmp');

	RPI_LCD_setFont($hash, 1);
	RPI_LCD_text($hash, 76, 34, 'Stopped!', 0);

	return undef;
}

=head2 RPI_LCD_Shutdown
	Title:    RPI_LCD_Shutdown
	Function: Implements DeleteFn function.
	Returns:  void
	Args:     named arguments:
		      -argument1 => hash: $hash hash of device addressed
=cut
sub RPI_LCD_Shutdown($$) {
	my ($hash) = @_;
	Log (1, "RPI_LCD_Shutdown");

	RPI_LCD_clearContent($hash);
	RPI_LCD_bmp($hash, 0, 14, '/opt/rpiLcdDaemon/images/fhem2.bmp');
	RPI_LCD_bmp($hash, 66, 15, '/opt/rpiLcdDaemon/images/fhem3.bmp');

	RPI_LCD_setFont($hash, 1);
	RPI_LCD_text($hash, 55, 35, 'Stopped!', 0);
}

=head2 RPI_LCD_Read
	Title:    RPI_LCD_Read
	Function: Implements ReadFn function,
	          called from the global loop, when the select for hash->{FD} reports data
	Returns:  nothing
	Args:     named arguments:
	          -argument1 => hash $hash: hash of device
=cut
sub RPI_LCD_Read($) {
	my ($hash) = @_;

	my $buffer = DevIo_SimpleRead($hash);

	if (defined($buffer)) {
		RPI_LCD_parseCommand($hash, $buffer);
	}
}

=head2 RPI_LCD_Set
	Title:    RPI_LCD_Set
	Function: Implements SetFn function.
	Returns:  string
	Args:     named arguments:
	          -argument1 => hash: $hash hash of device
	          -argument2 => array: @a argument array
=cut
sub RPI_LCD_Set($@) {
	my ($hash, @a) = @_;

	my $name =$a[0];
	my $cmd = $a[1];
	my $msg = '';

	return '"set ' . $name . '" needs one or more parameter' if(@a < 2);

	if(!defined($sets{$cmd})) {

		my $cmdList = '';
		foreach my $key (sort keys %sets) {
#			Log (1, " -- " . $key);
			if ($sets{$key} && $sets{$key} ne ' ') {
				$cmdList.= $key . ':' . $sets{$key} . ' ';
			} else {
				$cmdList.= $key . ' ';
			}
		}
		
		return 'Unknown argument ' . $cmd . ', choose one of ' . $cmdList;
	}

	if ($cmd eq 'cls') {
		RPI_LCD_clearContent($hash);

	} elsif ($cmd eq 'clearAll') {
		RPI_LCD_clearAll($hash);

	} elsif ($cmd eq 'line') {
		my $x1 = $a[2];
		my $y1 = $a[3];
		my $x2 = $a[4];
		my $y2 = $a[5];

		if( @a < 5 || (int($x1) ne $x1) || (int($y1) ne $y1) || (int($x2) ne $x2) || (int($y2) ne $y2) ) {
			return 'Wrong syntax: Use "set ' . $name . ' line x1 y1 x2 y2", x1, y1, x2, y2 must be numbers.';
		} else {
			RPI_LCD_line($hash, $x1, $y1, $x2, $y2);
		}

	} elsif ($cmd eq 'rect') {
		my $x1 = $a[2];
		my $y1 = $a[3];
		my $x2 = $a[4];
		my $y2 = $a[5];
		my $outline = !defined($a[6]) ? 1 : $a[6];

		if( @a < 5 || (int($x1) ne $x1) || (int($y1) ne $y1) || (int($x2) ne $x2) || (int($y2) ne $y2) || int($outline) ne $outline ) {
			return 'Wrong syntax: Use "set ' . $name . ' rect x1 y1 x2 y2 [outline]", x1, y1, x2, y2, outline must be numbers.';
		} else {
			RPI_LCD_rect($hash, $x1, $y1, $x2, $y2, $outline);
		}

	} elsif ($cmd eq 'circle') {
		my $x = $a[2];
		my $y = $a[3];
		my $r = $a[4];

		if( @a < 4 || (int($x) ne $x) || (int($y) ne $y) || (int($r) ne $r) ) {
			return 'Wrong syntax: Use "set ' . $name . ' circle x y r", x, y, r must be numbers.';
		} else {
			RPI_LCD_circle($hash, $x, $y, $r);
		}

	} elsif ($cmd eq 'text') {
		shift(@a);
		shift(@a);
		my $params = join(' ', @a);

		my ($x, $y, $string, $invert,$clearBg) = split(' ', $params);

		if($params =~ m/(.*){(.*)}(.*)/) {
			($x, $y) = split(' ', $1);
  			$string = '{' . $2 . '}';
  			($invert, $clearBg) = split(' ', $3);

		} elsif($params =~ m/(.*)['"](.*)['"](.*)/) {
			($x, $y) = split(' ', $1);
  			$string = $2;
  			($invert, $clearBg) = split(' ', $3);
		}
		
		$invert = !defined($invert) ? 0 : 1;
		$clearBg = !defined($clearBg) ? 1 : $clearBg;

		if( !$string || (int($x) ne $x) || (int($y) ne $y) || (int($invert) ne $invert) || (int($clearBg) ne $clearBg) ) {
			return 'Wrong syntax: Use "set ' . $name . ' text x y string [invert] [clearBg]", x, y, invert, clearBg must be numbers.';
		} else {
			RPI_LCD_text($hash, $x, $y, $string, $invert, $clearBg);
		}

	} elsif ($cmd eq 'bmp') {
		my $x = $a[2];
		my $y = $a[3];
		my $file = $a[4];

		if( @a < 4 || (int($x) ne $x) || (int($y) ne $y) ) {
			return 'Wrong syntax: Use "set ' . $name . ' bmp x y filename", x, y must be numbers.';
		} else {
			if (!(-e $file)) {
				return 'File "' . $file . '" cold not found!';
			} else {
				RPI_LCD_bmp($hash, $x, $y, $file);
			}
		}

	} elsif ($cmd eq 'date') {
		my $x = $a[2];
		my $y = $a[3];
		my $fontNum = $a[4];

		if( @a < 4 || (int($x) ne $x) || (int($y) ne $y) || (int($fontNum) ne $fontNum) ) {
			return 'Wrong syntax: Use "set ' . $name . ' date x y fontNum", x, y, fontNum, must be numbers.';
		} else {
			if (($fontNum) < 0 || int($fontNum) > 3) {
				return 'fontNumber must >= 0 and < 4';
			} else {
				RPI_LCD_date($hash, $x, $y, $fontNum);
			}
		}

	} elsif ($cmd eq 'time') {
		my $x = $a[2];
		my $y = $a[3];
		my $fontNum = $a[4];
		my $showSeconds = $a[5]; 

		if( @a < 5 || (int($x) ne $x) || (int($y) ne $y) || (int($fontNum) ne $fontNum) || (int($showSeconds) ne $showSeconds) ) {
			return 'Wrong syntax: Use "set ' . $name . ' time x y fontNum showSeconds", x, y, fontNum, showSeconds must be numbers.';
		} else {
			if (($fontNum) < 0 || int($fontNum) > 3) {
				return 'fontNumber must >= 0 and < 4';
			} else {
				RPI_LCD_time($hash, $x, $y, $fontNum, $showSeconds);
			}
		}

	} elsif ($cmd eq 'stopTime') {
		RPI_LCD_stopTime($hash);

	} elsif ($cmd eq 'stopDate') {
		RPI_LCD_stopDate($hash);

	} elsif ($cmd eq 'setFont') {
		my $fontNum = $a[2];
		if( @a < 2 || int($fontNum) < 0 || int($fontNum) > 3)  {
			return 'Wrong syntax: Use "set ' . $name . ' setFont fontNumber", fontNumber must >= 0 and < 4';
		} else {
			RPI_LCD_setFont($hash, $fontNum);
		}

	} elsif ($cmd eq 'led') {
		my $led = $a[2];
		my $value = $a[3];
		RPI_LCD_command($hash, 'setLed,' . $led . ',' . $value);

	} elsif ($cmd eq 'backlight') {
		if (($a[2] >= 0) && ($a[2] <= 255)) {
			my $backlight = ($a[2] >= 0) ? $a[2] : 0;
			$backlight = ($backlight <= 255) ? $backlight : 255;
			RPI_LCD_command($hash, 'setBacklight,' . $backlight);
		} else {
			return "Unknown argument $a[2], choose one of 1:2:3:4";
		}
	}

	return $msg;
}

#------------------------------------------------------------------------------
# RPI_LCD_command
#------------------------------------------------------------------------------
sub RPI_LCD_command(@) {
	my ($hash, $msg, $nonl) = @_;

	return if(!$hash || AttrVal($hash->{NAME}, "dummy", undef));
	my $name = $hash->{NAME};
	my $ll5 = GetLogLevel($name,5);

	Log (1, 'Rpi LCD command: '.$msg);

	$msg .= "\r\n";
	
	# writes command to daemon
	syswrite($hash->{TCPDev}, $msg) if($hash->{TCPDev});
}



###############################################################################

#------------------------------------------------------------------------------
# RPI_LCD_parseCommand
#------------------------------------------------------------------------------
sub RPI_LCD_parseCommand($$) {
	my ($hash, $command) = @_;

	if($command =~ m/ButtonStatus:/) {
		my $name = $hash->{NAME};

		my ($txtBtnStatus, $txtBtnCounter) = split(", ", $command);
		if (defined($txtBtnStatus) && $txtBtnStatus && defined($txtBtnCounter) && $txtBtnCounter) {
			my (undef, $btnStatus) = split(": ", $txtBtnStatus);
			my (undef, $btnCounter) = split(": ", $txtBtnCounter);
	
			$btnStatus = 0 if (!defined($btnStatus));
	
			RPI_LCD_processButtons($hash, $btnStatus);
	
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
}

#------------------------------------------------------------------------------
# RPI_LCD_processButtons
#------------------------------------------------------------------------------
sub RPI_LCD_processButtons($$) {
	my ($hash, $buttonState) = @_;

	if ($menuItemActive > 0) {
		if ( ($buttonState & 0b1000) > 0) { # End Menu Item (long middle press)
			$menuItemSelected = 0;
			$menuItemActive = 0;
			$menuOn = 0;
			RPI_LCD_contentDefault($hash);
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

		if ( ($buttonState & 0b1000) > 0 && $menuOn < 1) { # Activate menu (long middle press)
			$menuOn = 1;
			$menuItemSelected = 1;
			
			RPI_LCD_clearContent($hash);
			
			RPI_LCD_refreshMenu($hash);

		} elsif ( ($buttonState & 0b1000) > 0 && $menuOn == 1) {	# Deactivate menu (long middle press)
			$menuOn = 0;
			$menuItemSelected = 0;
			RPI_LCD_contentDefault($hash);

		} elsif (($buttonState & 0b100) > 0) {						# Select Menu Item (short middle press)
			$menuItemActive = $menuItemSelected;
		}

		if ($menuOn && $menuItemActive == 0) {
			if ( ($buttonState & 0b10000) > 0) {					# Up (activated Menu)
				$menuItemSelected++
			}

			if ( ($buttonState & 0b1) > 0) {						# Down (activated Menu)
				$menuItemSelected--
			}

			RPI_LCD_refreshMenu($hash);
		}
	}
}

#------------------------------------------------------------------------------
# RPI_LCD_refreshMenu
#------------------------------------------------------------------------------
sub RPI_LCD_refreshMenu($) {
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
		RPI_LCD_text($hash, 0, $line, $menuItemns{$i}{item}, $invert);

		$line = $line + 10;
	}
}

=head2 RPI_LCD_init
	Function: Init the LCD
	Returns:  nothing
	Args:     named arguments:
	          -argument1 => hash: hash of device addressed
=cut
sub RPI_LCD_init($) {
	my ($hash) = @_;

	RPI_LCD_command($hash, "cls");
	RPI_LCD_command($hash, "setBacklight,30");

	RPI_LCD_setHeader($hash);
	RPI_LCD_contentDefault($hash);
}

#------------------------------------------------------------------------------
# RPI_LCD_setHeader
#------------------------------------------------------------------------------
sub RPI_LCD_setHeader($) {
	my ($hash) = @_;

	RPI_LCD_date($hash, 18, -3, 1);
	RPI_LCD_time($hash, 88, -3, 1, 0);

	RPI_LCD_rect($hash, 2, 4, 12, 8, 1);
	RPI_LCD_line($hash, 0, 4, 7, 0);
	RPI_LCD_line($hash, 7, 0, 14, 4);
	RPI_LCD_line($hash, 0, 4, 14, 4);

	RPI_LCD_line($hash, 0, 10, 128, 10);
}

#------------------------------------------------------------------------------
# RPI_LCD_contentDefault
#------------------------------------------------------------------------------
sub RPI_LCD_contentDefault($) {
	my ($hash) = @_;

	RPI_LCD_clearContent($hash);
	RPI_LCD_bmp($hash, 0, 14, '/opt/rpiLcdDaemon/images/fhem2.bmp');
	RPI_LCD_bmp($hash, 66, 15, '/opt/rpiLcdDaemon/images/fhem3.bmp');

	RPI_LCD_setFont($hash, 0);
	RPI_LCD_text($hash, 76, 34, 'Home', 0);
	RPI_LCD_text($hash, 60, 44, 'Automation', 0);
	RPI_LCD_text($hash, 70, 54, 'Server', 0);
}

#------------------------------------------------------------------------------
# RPI_LCD_clearContent
#------------------------------------------------------------------------------
sub RPI_LCD_clearContent($) {
	my ($hash) = @_;

	RPI_LCD_rect($hash, 0, 11, 128, 64, 0);
}

#------------------------------------------------------------------------------
# RPI_LCD_clearAll
#------------------------------------------------------------------------------
sub RPI_LCD_clearAll($) {
	my ($hash) = @_;

	RPI_LCD_rect($hash, 0, 0, 128, 64, 0);
}

#------------------------------------------------------------------------------
# RPI_LCD_text
#------------------------------------------------------------------------------
sub RPI_LCD_text($$$$$;$) {
	my ($hash, $x, $y, $text, $invert, $clearBg) = @_;
	$clearBg = !defined($clearBg) ? 1 : $clearBg;
	$clearBg = $clearBg ? 1 : 0;

	if($text =~ m/^{(.*)}$/) {
		$text = eval $1;
		if($@) {
			Log (1, 'Error evaluating text: ' . $@);
		}
	}

	if ($text) {		
		RPI_LCD_command($hash, 'text,' . $x . ',' . $y . ',' . $text . ',' . $invert . ',' . $clearBg);
	}
}

#------------------------------------------------------------------------------
# RPI_LCD_bmp
#------------------------------------------------------------------------------
sub RPI_LCD_bmp($$$$) {
	my ($hash, $x, $y, $file) = @_;

	RPI_LCD_command($hash, 'bmp,' . $x . ',' . $y . ',' . $file);
}

#------------------------------------------------------------------------------
# RPI_LCD_line
#------------------------------------------------------------------------------
sub RPI_LCD_line($$$$$) {
	my ($hash, $x1, $y1, $x2, $y2) = @_;

	RPI_LCD_command($hash, 'line,' . $x1 . ',' . $y1 . ',' . $x2 . ',' . $y2);
}

#------------------------------------------------------------------------------
# RPI_LCD_rect
#------------------------------------------------------------------------------
sub RPI_LCD_rect($$$$$;$) {
	my ($hash, $x1, $y1, $x2, $y2, $outline) = @_;
	$outline = !defined($outline) ? 1 : $outline;

	RPI_LCD_command($hash, 'rect,' . $x1 . ',' . $y1 . ',' . $x2 . ',' . $y2 . ',' . $outline);
}

#------------------------------------------------------------------------------
# RPI_LCD_circle
#------------------------------------------------------------------------------
sub RPI_LCD_circle($$$$) {
	my ($hash, $x, $y, $r) = @_;

	RPI_LCD_command($hash, 'circle,' . $x . ',' . $y . ',' . $r);
}

#------------------------------------------------------------------------------
# RPI_LCD_rect
#------------------------------------------------------------------------------
sub RPI_LCD_setFont($$) {
	my ($hash, $fontNum) = @_;

	RPI_LCD_command($hash, 'setFont,' . $fontNum);
}

#------------------------------------------------------------------------------
# RPI_LCD_date
#------------------------------------------------------------------------------
sub RPI_LCD_date($$$$) {
	my ($hash, $x, $y, $fontNumber) = @_;

	RPI_LCD_command($hash, 'date,' . $x . ',' . $y . ',' . $fontNumber . ',0');
}

#------------------------------------------------------------------------------
# RPI_LCD_time
#------------------------------------------------------------------------------
sub RPI_LCD_time($$$$$) {
	my ($hash, $x, $y, $fontNumber, $showSeconds) = @_;

	RPI_LCD_command($hash, 'time,' . $x . ',' . $y . ',' . $fontNumber . ',' . $showSeconds);
}

#------------------------------------------------------------------------------
# RPI_LCD_stopTime
#------------------------------------------------------------------------------
sub RPI_LCD_stopTime($) {
	my ($hash) = @_;
	RPI_LCD_command($hash, 'stopTime');
}

#------------------------------------------------------------------------------
# RPI_LCD_stopDate
#------------------------------------------------------------------------------
sub RPI_LCD_stopDate($) {
	my ($hash) = @_;
	RPI_LCD_command($hash, 'stopDate');
}

#------------------------------------------------------------------------------
# RPI_LCD_menuDisplayIP
#------------------------------------------------------------------------------
sub RPI_LCD_menuDisplayIP() {
	my ($hash, $buttonState) = @_;

	RPI_LCD_clearContent($hash);
	RPI_LCD_setFont($hash, 0);
	RPI_LCD_text($hash, 0, 13, 'IP-Adresse(n):', 0);

	my $cmdLine = 'ip -o addr show | awk \'/inet/ {print $2, $3, $4}\'';
	my @ips = `$cmdLine`;
	my $line = 23;
	foreach my $ipLine (@ips) {
		my ($interface, undef, $ipParts) = split(' ', $ipLine);
		my ($ip) = split('/', $ipParts);
		if ($interface ne 'lo') {
			RPI_LCD_text($hash, 0, $line, $interface . ':' . $ip, 0);
			$line = $line +10;
		}

		Log (1, "IP: $interface: $ip");
	}
}


#------------------------------------------------------------------------------
# RPI_LCD_menuShutdown
#------------------------------------------------------------------------------
sub RPI_LCD_menuShutdown() {
	my ($hash, $buttonState) = @_;

	RPI_LCD_clearContent($hash);
	RPI_LCD_setFont($hash, 1);
	RPI_LCD_text($hash, 23, 25, 'Shutdown', 0);

	`sudo /sbin/halt`;
}

1;

# set rpiLCD text 12 12 hallo_erstmal 0 1
# set rpiLCD rect 0 0 30 63

=pod
=begin html

<a name="00_RPI_LCD.pm"></a>
	<h3>tbd.</h3>
	<p> FHEM module to commmunicate liquid crystal display on Raspberry Pi</p>
	<ul>
		<li>...</li>
		<li>...</li>
		<li>...</li>
	</ul>
=end html
=cut

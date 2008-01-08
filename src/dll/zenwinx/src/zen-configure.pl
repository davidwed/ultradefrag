#!/usr/bin/perl
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# This is the ZenWINX build configurator.
# Copyright (c) 2007,2008 by Dmitri Arkhangelski (dmitriar@gmail.com).
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

use strict;
use Tk;

my %opts = (
	'ZENWINXVER'=>' ',
	'WINDDKBASE'=>' ',
	'MINGWBASE'=>' ',
	'SEVENZIP_PATH'=>' ',
	'MSVSBIN'=>' ',
	);
my @fields;
my $p;

# Specifying undef as the -popover location means the center of the screen.
# More details at http://aspn.activestate.com/ASPN/Mail/Message/perl-tk/1859979.
my $mw = MainWindow->new(-title => 'ZenWINX build configurator',-popover => undef);
$mw->withdraw(); # prevents the window jump from its default location to the -popover location
my(@relief) = qw/-relief sunken/;
my $label1 = $mw->Label(-text => 'ZenWINX version:');
my $zver = $mw->Entry(@relief,-textvariable=>\$opts{'ZENWINXVER'});
my $label2 = $mw->Label(-text => 'MinGW base path:');
my $mingw = $mw->Entry(@relief,-textvariable=>\$opts{'MINGWBASE'});
my $label3 = $mw->Label(-text => 'Windows DDK base path:');
my $ddk = $mw->Entry(@relief,-textvariable=>\$opts{'WINDDKBASE'});
my $label4 = $mw->Label(-text => '7-Zip root path:');
my $szip = $mw->Entry(@relief,-textvariable=>\$opts{'SEVENZIP_PATH'});
my $label5 = $mw->Label(-text => 'Visual Studio bin path:');
my $vs = $mw->Entry(@relief,-textvariable=>\$opts{'MSVSBIN'});
my $patch_btn = $mw->Button(-text => 'Apply patch to MinGW',
                            -command => sub {mingw_patch();});
my $save_btn = $mw->Button(-text => 'Save', -command => sub {save_opts();});
my $exit_btn = $mw->Button(-text => 'Exit', -command => sub {exit});

$label1->form(-top => '%0', -left => '%0', -right => '%30');
$zver->form(-top => '%0', -left => $label1, -right => '%100');
$label2->form(-top => $label1, -left => '%0', -right => '%30');
$mingw->form(-top => $zver, -left => $label2, -right => '%100');
$label3->form(-top => $label2, -left => '%0', -right => '%30');
$ddk->form(-top => $mingw, -left => $label3, -right => '%100');
$label4->form(-top => $label3, -left => '%0', -right => '%30');
$szip->form(-top => $ddk, -left => $label4, -right => '%100');
$label5->form(-top => $label4, -left => '%0', -right => '%30');
$vs->form(-top => $szip, -left => $label5, -right => '%100');
$patch_btn->form(-top => $label5, -left => '%0', -right => '%30');
$save_btn->form(-top => $vs, -left => $patch_btn, -right => '%65');
$exit_btn->form(-top => $vs, -left => $save_btn, -right => '%100');

# read configuration from the setvars_zenwinx.cmd file and make backup
open(BKFILE,'> .\\SETVARS_ZENWINX.BK')
	or msg_and_die('Can\'t make backup for SETVARS_ZENWINX.CMD file: $!');
if(open(CFGFILE,'.\\SETVARS_ZENWINX.CMD')){
	foreach (<CFGFILE>){
		print BKFILE $_;
		chomp($_);
		@fields = split(/=/);
		if(@fields == 2){
			$p = substr($fields[0],4);
			if($p ne undef && $opts{$p} ne undef){
				$opts{$p} = $fields[1];
			}
		}
	}
	close CFGFILE;
	close BKFILE;
}else{
	close BKFILE;
	msg_and_die('Can\'t open SETVARS_ZENWINX.CMD file!');
}

$mw->Popup();
MainLoop;

sub msg {
	$mw->messageBox(
		-icon => 'error', -type => 'OK',
		-title => 'Error!', -message => $_[0]
		);
}

sub msg_and_die {
	msg($_[0]);
	die($_[0]);
}

sub save_opts {
	my $key;
	my @v;

	open(CFGFILE,'> SETVARS_ZENWINX.CMD')
		or msg_and_die('Can\'t open SETVARS_ZENWINX.CMD file: $!');
	print CFGFILE "\@echo off\necho Set common environment variables...\n";
	@v = split(/\./,$opts{'ZENWINXVER'});
	print CFGFILE "set ZENWINX_VERSION=$v[0],$v[1],$v[2],0\n";
	print CFGFILE "set ZENWINX_VERSION2=\"$v[0], $v[1], $v[2], 0\\0\"\n";
	foreach $key (keys (%opts)) {
			print CFGFILE "set $key=$opts{$key}\n";
	}
	close CFGFILE;
}

sub mingw_patch {
my @command = ("cmd.exe","/C","mingw_patch.cmd","");
$command[@command-1] = $opts{'MINGWBASE'};
system(@command) == 0 or msg('Can\'t apply patch to MinGW installation!');
}

__END__

#!/usr/bin/perl
#  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
# This is the Ultra Defragmenter build configurator.
# Copyright (c) 2007 by Dmitri Arkhangelski (dmitriar@gmail.com).
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
#  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

use strict;
use Tk;

my %opts = (
	'ULTRADFGVER'=>' ',
	'WINDDKBASE'=>' ',
	'NETRUNTIMEPATH'=>' ',
	'NSISDIR'=>' ',
	'SEVENZIP_PATH'=>' ',
	'MSVSBIN'=>' ',
	'DDKINCDIR'=>' ',
	);

# Specifying undef as the -popover location means the center of the screen.
# More details at http://aspn.activestate.com/ASPN/Mail/Message/perl-tk/1859979.
my $mw = MainWindow->new(-title => 'UltraDefrag build configurator',-popover => undef);
$mw->withdraw(); # prevents the window jump from its default location to the -popover location
my(@relief) = qw/-relief sunken/;
my $label1 = $mw->Label(-text => 'UltraDefrag version:');
my $ver = $mw->Entry(@relief);
my $label2 = $mw->Label(-text => 'Windows DDK base path:');
my $ddk = $mw->Entry(@relief);
my $label3 = $mw->Label(-text => '.NET runtime path:');
my $net = $mw->Entry(@relief);
my $label4 = $mw->Label(-text => 'NSIS root path:');
my $nsis = $mw->Entry(@relief);
my $label5 = $mw->Label(-text => '7-Zip root path:');
my $szip = $mw->Entry(@relief);
my $label6 = $mw->Label(-text => 'Visual Studio bin path:');
my $vs = $mw->Entry(@relief);
my $label7 = $mw->Label(-text => 'ReactOS include path:');
my $ros = $mw->Entry(@relief);
my $save_btn = $mw->Button(-text => 'Save', -command => sub {save_opts();});
my $exit_btn = $mw->Button(-text => 'Exit', -command => sub {exit});

$label1->form(-top => '%0', -left => '%0', -right => '%30');
$ver->form(-top => '%0', -left => $label1, -right => '%100');
$label2->form(-top => $label1, -left => '%0', -right => '%30');
$ddk->form(-top => $ver, -left => $label2, -right => '%100');
$label3->form(-top => $label2, -left => '%0', -right => '%30');
$net->form(-top => $ddk, -left => $label3, -right => '%100');
$label4->form(-top => $label3, -left => '%0', -right => '%30');
$nsis->form(-top => $net, -left => $label4, -right => '%100');
$label5->form(-top => $label4, -left => '%0', -right => '%30');
$szip->form(-top => $nsis, -left => $label5, -right => '%100');
$label6->form(-top => $label5, -left => '%0', -right => '%30');
$vs->form(-top => $szip, -left => $label6, -right => '%100');
$label7->form(-top => $label6, -left => '%0', -right => '%30');
$ros->form(-top => $vs, -left => $label7, -right => '%100');
$save_btn->form(-top => $ros, -left => '%30', -right => '%65');
$exit_btn->form(-top => $ros, -left => $save_btn, -right => '%100');

# read configuration from the setvars.cmd file and make backup
open(BKFILE,'> .\\SETVARS.BK')
	or msg_and_die('Can\'t make backup for SETVARS.CMD file: $!');
if(open(CFGFILE,'.\\SETVARS.CMD')){
	my @fields;
	foreach (<CFGFILE>){
		print BKFILE $_;
		chomp($_);
		@fields = split(/=/);
		if(@fields == 2){
			my $p = substr($fields[0],4);
			if($p ne undef && $opts{$p} ne undef){
				$opts{$p} = $fields[1];
			}
		}
	}
	close CFGFILE;
	close BKFILE;
}else{
	close BKFILE;
	msg_and_die('Can\'t open SETVARS.CMD file!');
}
# the following is not a best solution, because on Windows we don't have the fork() system call.
#my @bk_cmd = ("cmd.exe","/C","copy","/Y","SETVARS.CMD","SETVARS.BK");
#system(@bk_cmd) == 0 or msg_and_die('Can\'t make backup for SETVARS.CMD file!');

$ver->insert(0,$opts{'ULTRADFGVER'});
$ddk->insert(0,$opts{'WINDDKBASE'});
$net->insert(0,$opts{'NETRUNTIMEPATH'});
$nsis->insert(0,$opts{'NSISDIR'});
$szip->insert(0,$opts{'SEVENZIP_PATH'});
$vs->insert(0,$opts{'MSVSBIN'});
$ros->insert(0,$opts{'DDKINCDIR'});

$mw->Popup();
MainLoop;

sub msg_and_die {
	$mw->messageBox(
		-icon => 'error', -type => 'OK',
		-title => 'Error!', -message => $_[0]
		);
	die($_[0]);
}

sub save_opts {
	my $key;
	open(CFGFILE,'> .\\SETVARS.CMD')
		or msg_and_die('Can\'t open SETVARS.CMD file!');
	print CFGFILE "\@echo off\necho Set common environment variables...\n";
	my @v = split(/\./,$opts{'ULTRADFGVER'});
	print CFGFILE "set VERSION=$v[0],$v[1],$v[2],0\n";
	print CFGFILE "set VERSION2=\"$v[0], $v[1], $v[2], 0\\0\"\n";
	foreach $key (keys (%opts)) {
		print CFGFILE "set $key=$opts{$key}\n";
	}
	close CFGFILE;
}

__END__

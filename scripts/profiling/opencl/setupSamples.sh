#!/bin/bash
#
# Copyright (C) 2014  Politecnico di Milano
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>


# ========================== Setup data ==================================

BOSP_BASE=${BOSP_BASE:-"/home/gmassari/BOSP"}
#BOSP_BASE=${BOSP_BASE:-"/opt/BOSP"}
DATETIME=`date +%Y%m%d_%H%M%S`
DATETIME_STR=`date`

NUMRUN=${NUMRUN:-"3"}
NUMINST=${NUMINST:-"1 2 3"}

OUTDIR=${OUTDIR:-$BOSP_BASE"/out/var/ocl"}
OUTFILENAME="BbqueOCLStats"
SUBDIR_PREFIX=$DATETIME
DATADIR_PREFIX=${DATADIR_PREFIX:-"/tmp"}

XORG_CONF="/etc/X11/xorg.conf_gpu"
OCL_PROF="o0:M"
SEL=""

# Frequency values
GPU_CORE_FREQ=300
GPU_MEM_FREQ=400
CPU_FREQ_GOV="performance"
KERN_USLEEP_PERC="0 5 10 15 20 25"
#KERN_USLEEP_PERC="30 35 40 50"

#Extra qualifiers
EXTRA="Sleep"

# ========================== Samples data ==================================
ocl_names=(nbody stereomatch fluidsimulation2D montecarlo mandelbrot)
AMD_SAMPLES="NBody FluidSimulation2D MonteCarloAsianDP Mandelbrot"
#AMD_SAMPLES="MonteCarloAsianDP"
AMD_TIME_OPT=""

SAMPLES=("nbody stereomatch fluidsimulation2D montecarlo mandelbrot")
#SAMPLES="nbody"
#SAMPLES="montecarlo"
#SAMPLES="nbody stereomatch fluidsimulation2D montecarlo mandelbrot"
#SAMPLES="stereomatch fluidsimulation2D"
#SAMPLES="fluidsimulation2D"
SAMPLES="mandelbrot"
ARGS=("" "-x" "" "" "-c" "")

# Number of iterations per sample execution
#          nbody
#          |   stereomatch
#          |   |   fluidsim
#          |   |   |   montecarlo
#          |   |   |   |  mandelbrot
NUMITER=(0 100 5 100 3 1000)
#NUMITER=(0 25 5 10 3 100)
#NUMITER=(0 10 20 90 1)
#TOTRUNS=$(($NUMRUN*${#SAMPLES[@]}))
#TOTRUNS=$(($NUMRUN*4))


# Number of NBody particles
NB_PARAMS=(32768)
#NB_PARAMS=(32768 8192 2048)
#NB_PARAMS=(32768 16384 8192 4096 2048 1024)
#NB_PARAMS=(16384)

# Number of MonteCarloAsianDP steps
#MC_PARAMS=(8 16 32 64 128 256)
#MC_PARAMS=(8 32 128)
#MC_PARAMS=(256)
MC_PARAMS=(64)


# $1 = sample
# $2 = current number of run
# $3 = number of instances
# $4 = application parameter value
function print_test_header {
	printf "============================================\n"
	printf "| BBQ Test run    : %-3d / %-16d | \n" $2 $NUMRUN
	echo   "--------------------------------------------"
	printf "| SAMPLE          : %-17s      | \n" $1
	printf "| Instances       : %-17d      | \n" $3
	printf "| Parameter value : %-17d      | \n" $4
	printf "============================================  \n"
}


function setup {
	# Recipe setup for the experiment
	BOSP_BASE=$BOSP_BASE $BOSP_BASE/initexp.sh $1
	if [ $? != 0 ]; then
		exit $?
	fi

	# Block GPUs frequency
	(amdconfig --adapter=all --od-setclocks=$GPU_CORE_FREQ,$GPU_MEM_FREQ --input=$XORG_CONF)

	# Set cpufreq governor to 'performance'
	echo "cpufreq governors:"
	for i in `seq 0 7`;do
		cpu_gov=$(cat /sys/devices/system/cpu/cpu"$i"/cpufreq/scaling_governor)
		echo "cpu"$i": "$cpu_gov
		if [ "$cpu_gov" != "$CPU_FREQ_GOV" ]; then
			echo "Please set the cpufreq governor to "$CPU_FREQ_GOV"!"
			exit
		fi
	done

	# Create output directory
	echo $OUTDIR
	if [ ! -d "$OUTDIR" ]; then
		mkdir $OUTDIR
	fi

	OUTDIR=$OUTDIR"/"$SUBDIR_PREFIX
	if [ ! -d "$OUTDIR" ]; then
		mkdir $OUTDIR
	fi

	TESTDIR=$OUTDIR"/GPUc"$GPU_CORE_FREQ"m"$GPU_MEM_FREQ"-CPU"$CPU_FREQ_GOV"-"$EXTRA
	if [ ! -d "$TESTDIR" ]; then
		mkdir $TESTDIR
	fi
	echo "Output directory: " $TESTDIR

	# Timing/profiling enabled?
	if [ $OCL_PROF ]; then
		AMD_TIME_OPT="-t"
	fi
}


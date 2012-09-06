function fail
{
    echo "FAIL: $@"
    exit 1
}

function verbose
{
    local level=$1
    shift;
    if [ $VERBOSE_LEVEL -ge $level ]; then
	echo $@
    fi
}

function run_cfg
{
    #   -nsub : number of submeshes
    local NS=$1
    #   -nt_d : number of threads to decompose each submesh
    local NTD=$2
    #   -nt_sm : number of threads to process the submeshes
    local NTSM=$3
    #   -nt_a : number of threads to assemble each submesh
    local NTA=$4
    #   -nt_m : number of threads to multiply
    local NTM=$5
    #   -p : plevel
    local P=$6

    local NAME=$7

    local EXTRAARGS=$8

    BASEOUT="pthread_vs_tbb.$NAME.@REAL_TYPE@.p$P.nsub$NS.nt_a.$NTA.nt_d.$NTD.nt_m.$NTM.nt_sm.$NTSM"
    
    IF="cubo1.p$P.nsub$NS.t.@REAL_TYPE@.bin.ckpt1"
    OF="cubo1.p$P.nsub$NS.t.@REAL_TYPE@.bin.ckpt3"
    GOLDEN="@PERFTEST_DATA_DIR@/SubStruct/outputs/$OF"
    MD5F="@PERFTEST_DATA_DIR@/SubStruct/outputs/$OF.md5"

    ASSRDT="$BASEOUT.ass.rdt"
    CRERDT="$BASEOUT.cre.rdt"

    CMD="$APP -bc -cf1 @PERFTEST_DATA_DIR@/SubStruct/inputs/$IF -dc3 $OF -st3 -ass_rdt $ASSRDT -cre_rdt $CRERDT" 
    CMD="$CMD -chk_c3_md5 $MD5F"
    CMD="$CMD -nsub $NS -nt_a $NTA -nt_d $NTD -nt_m $NTM -nt_sm $NTSM -p $P $EXTRAARGS"
    verbose 1 "cmd: $CMD"

    [ -f "$OF" ] && rm "$OF" # Remove if exists

    /usr/bin/time $TIMEARGS $CMD &> "$BASEOUT.output.txt"
    RET=$?

    if [ $RET != 0 ]; then
        echo "[ERROR]: $APP tool returned $RET."
        ERRORS=$((ERRORS+1))
    else
        # Check output file.
        
        # Side by side
        # DIFFOPTIONS="--suppress-common-lines -y -W 100"
        # Regular diff
        DIFFOPTIONS="--normal"
        
        if diff $DIFFOPTIONS "$OF" "$GOLDEN" > "$BASEOUT.diff" ; then
            echo "[OK]: $OF matches the golden output."
            rm "$OF" # Remove the output file.
            OKS=$((OKS+1))
        else
            echo "[ERROR]: Checkpoint file \"$OF\" differs from golden. Differences at $BASEOUT.diff."
            ERRORS=$((ERRORS+1))
        fi
    fi        
}

# ==================================
APP="@PERFTEST_APPS_DIR@/SubStruct/Perf-SubStruct"

# Select the time command arguments
if /usr/bin/time -l ls &> /dev/null; then
    TIMEARGS="-l"
elif /usr/bin/time --verbose ls &> /dev/null; then
    TIMEARGS="--verbose"
fi
echo "TIMEARGS = $TIMEARGS"

# ==================================
# Main
# ==================================

# Run configuration
VERBOSE_LEVEL=1

# Base number of threads for each step.
NTHREADS=4
L1NTHREADS=$NTHREADS
L2NTHREADS=$NTHREADS

NCORES=`sysctl hw.physicalcpu`

verbose 1 "pthread vs tbb assemble: cubo"

# Check if application binary exists
[ -f $APP ] || fail "Application $APP is not a file"

verbose 1 "app: $APP"
verbose 1 " NCORES = $NCORES"
verbose 1 " L1 NTHREADS =  / $L1NTHREADS" 
verbose 1 " L2 NTHREADS =  / $L2NTHREADS" 

ERRORS=0
OKS=0

echo "WARNING: a ferramenta está quebrando com NSUB == 1"

for NSUB in 2 4 8 16 32 64; do
    echo "Start at checkpoint 1, dump checkpoint 3 and stop: plevel = $PLEVEL"

    # L1 Serial, L2 TBB
    #       subm  L1(ntd ntsm) L2(nta) nt_mul plevel  name            "extra" 
    run_cfg $NSUB      0    0       0       0      2  "L1_ser_L2_TBB" "-pair_tbb"  

    # L1 Serial, L2 PThreads
    #       subm  L1(ntd ntsm) L2(nta) nt_mul plevel                 "extra" 
    run_cfg $NSUB      0    0  $NCORES      0      2  "L1_ser_L2_pth" ""  

    # L1 Serial, L2 serial
    #       subm  L1(ntd ntsm) L2(nta) nt_mul plevel                 "extra" 
    run_cfg $NSUB      0    0       0       0      2  "L1_ser_L2_ser" ""  

    # L1 TBB, L2 TBB
    #       subm  L1(ntd ntsm) L2(nta) nt_mul plevel                 "extra" 
    run_cfg $NSUB      0    0       0       0      2  "L1_TBB_L2_TBB" "-dohr_tbb -pair_tbb"  

    # L1 TBB, L2 PThread
    #       subm  L1(ntd ntsm) L2(nta) nt_mul plevel                 "extra" 
    run_cfg $NSUB      0    0  $NCORES      0      2  "L1_TBB_L2_pth" "-dohr_tbb"  

    # L1 PThread, L2 TBB
    #       subm  L1(ntd ntsm) L2(nta) nt_mul plevel                 "extra" 
    run_cfg $NSUB $NCORES $NCORES    0      0      2  "L1_pth_L2_TBB" "-pair_tbb"  

    # L1 PThread, L2 PThread
    #       subm  L1(ntd ntsm) L2(nta) nt_mul plevel                 "extra" 
    run_cfg $NSUB $NCORES $NCORES $NCORES   0      2  "L1_pth_L2_pth" ""  

    # L1 PThread, L2 Serial
    #       subm  L1(ntd ntsm) L2(nta) nt_mul plevel                 "extra" 
    run_cfg $NSUB $NCORES $NCORES   0       0      2  "L1_pth_L2_ser" ""  

    # L1 TBB, L2 Serial
    #       subm  L1(ntd ntsm) L2(nta) nt_mul plevel                 "extra" 
    run_cfg $NSUB      0    0       0       0      2  "L1_TBB_L2_ser" "-dohr_tbb"  

done

echo "# of ERROR: $ERRORS"
echo "# of OK   : $OKS"
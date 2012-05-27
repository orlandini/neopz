
VERBOSE_LEVEL=1

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

APP="@PERFTEST_APPS_DIR@/Substruct/Perf-SubStruct"

# Main
verbose 1 "perf01 - substruct performance test: cubo1 p1 1st step."
verbose 1 "app: $APP"

# Check if application binary exists
[ -f $APP ] || fail "Application $APP is not a file"

ERRORS=0
OKS=0

echo "Start at checkpoint 1 and dump checkpoint 2"
for ns in 1 2 4 16 32 64; do

  IF="cubo1.p1.nsub$ns.t.ckpt1"
  OF1="cubo1.p1.nsub$ns.t.ckpt2"
  OF2="cubo1.p1.nsub$ns.t.ckpt3"
  CMD="$APP -cf1 @PERFTEST_DATA_DIR@/Substruct/inputs/$IF -dc2 $OF1 -dc3 $OF2 " 
  verbose 1 "cmd: $CMD"
  /usr/bin/time -l $CMD &> "cubo1.ckpt1.p1.nsub$ns.output.txt"
  GOLDEN1="@PERFTEST_DATA_DIR@/Substruct/outputs/$OF1"
  GOLDEN2="@PERFTEST_DATA_DIR@/Substruct/outputs/$OF2"

  # Side by side
  # DIFFOPTIONS="--suppress-common-lines -y -W 100"
  # Regular diff
  DIFFOPTIONS="--normal"

  if ! diff $DIFFOPTIONS "$OF1" "$GOLDEN1" > "$OF1.diff"; then
      echo "[ERROR]: Checkpoint file \"$OF1\" differs from golden. Differences at $OF1.diff."
      ERRORS=$((ERRORS+1))
  else
      echo "[OK]: $OF1 matches the golden output."
      rm -f "$OF1.diff"
      OKS=$((OKS+1))
  fi
  if ! diff $DIFFOPTIONS "$OF2" "$GOLDEN2" > "$OF2.diff"; then
      echo "[ERROR]: Checkpoint file \"$OF2\" differs from golden. Differences at $OF2.diff."
      ERRORS=$((ERRORS+1))
  else
      echo "[OK]: $OF2 matches the golden output."
      rm -f "$OF2.diff"
      OKS=$((OKS+1))
  fi

done

echo "Start at checkpoint 2 and dump checkpoint 3"
for ns in 1 2 4 16 32 64; do

  IF="cubo1.p1.nsub$ns.t.ckpt2"
  OF="cubo1.p1.nsub$ns.t.ckpt3"
  CMD="$APP -cf2 @PERFTEST_DATA_DIR@/Substruct/inputs/$IF -dc3 $OF " 
  verbose 1 "cmd: $CMD"
  /usr/bin/time -l $CMD &> "cubo1.ckpt2.p1.nsub$ns.output.txt"
  GOLDEN="@PERFTEST_DATA_DIR@/Substruct/outputs/$OF"

  # Side by side
  # DIFFOPTIONS="--suppress-common-lines -y -W 100"
  # Regular diff
  DIFFOPTIONS="--normal"

  if diff $DIFFOPTIONS "$OF" "$GOLDEN" > "$OF.diff" ; then
      echo "[OK]: $OF matches the golden output."
      OKS=$((OKS+1))
  else
      echo "[ERROR]: Checkpoint file \"$OF\" differs from golden. Differences at $OF.diff."
      ERRORS=$((ERRORS+1))
  fi

done


echo "# of ERROR: $ERRORS"
echo "# of OK   : $OKS"

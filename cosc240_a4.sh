#!/usr/bin/env bash

set -Eeuo pipefail
trap cleanup SIGINT SIGTERM ERR EXIT

usage() {
  cat <<EOF
Usage: $(basename "${BASH_SOURCE[0]}") [-h] [-v] [-d] [-s simulator] [-o output] [-e schedules] [-l solution] [-u submissions] [-k] [-n] [-a] [-1] [-t timeout] [-m marks] [-c competition | algorithm1] [algorithmN...]

A program that does the following tasks for the COSC240 scheduling assessment:

* Collect valid spec_schedules into a single location
* Generate output from different submission's scheduling algorithms
* Compare output from different submission's scheduling algorithms to the expected output
* Collect average turnaround times from different submissions into a single location
* Rank averages to give a score for different submissions

Available options:

-h, --help             Print this help and exit
-v, --verbose          Print script debug info
-d, --debug            Print simulator debug info
-s, --simulator        Path to a C file that implements the simulator (defaults to "simulator.c")
-o, --output           Path to directory where output should be stored/used (defaults to "output")
-e, --schedules        Path to where schedules should be copied/used (defaults to "schedules")
-l, --solution         Path to solution code that produces the correct outputs (defaults to "algorithms")
-u, --submissions      Path to a directory that contains extracted submissions (default) or a single submission (when combined with the -1 option) (defaults to "submissions")
-k, --collect          Collect schedules from each submission into the schedules directory (by default this doesn't happen)
-n, --no-generate      Do not generate output for the submissions (defaults to generating output)
-a, --generate-answers Generate known correct output from the solution code (see "-l" option) (default to not generating known correct output)
-1, --single           Treat submissions as a single submission (defaults to assuming submissions is a directory containing multiple submissions)
-t, --timeout          The timeout to use when executing the simulator (defaults to "1m" for 1 minute)
-m, --marks            The marks attainable for correctly generating the output for each algorithm (defaults to 0, meaning marks are not output, and any generated output is not compared to the solution's output; if a larger integer is used, it calculates the marks as a percentage of correct outputs multiplied by this value)
-c, --competition      The marks attainable for winning the competition (if 0, competition is not run) (defaults to 0)

algorithm1 - algorithmN are the algorithms to process (either -c or at least one algorithm is required)

Example:
$(basename "${BASH_SOURCE[0]}") -u submissions/submission -1 fcfs

is identical to

$(basename "${BASH_SOURCE[0]}") -s simulator.c -o output -e schedules -l algorithms -u submissions/submission -1 -t "1m" fcfs

and will execute the simulator using the fcfs algorithm implemented in "submissions/submission/fcfs.c" with each of the schedules in the "schedules" directory, writing output to "output"

Example:
$(basename "${BASH_SOURCE[0]}") -m 5 fcfs

will execute the simulator for each "fcfs.c" implemented in each subdirectory of "submissions" with each of the schedules in the "schedules" directory, writing output to "output", and comparing the results of each to those of the code implemented in "algorithms/fcfs.c", giving up to five marks for each submission, based on how many of the generated outputs match those of the solution code

Example:
$(basename "${BASH_SOURCE[0]}") -c 5

will execute the simulator for each "custom.c" implemented in each subdirectory of "submissions" with each of the schedules in the "schedules" directory, writing output to "output", and ranking the results of each implementation, giving up to five marks for each submission, based on its ranking

EOF
}

# Set up a fresh simulator in a temporary directory (name of which will be stored in $sim_dir)
# Parameters:
#  $1 The directory containing the file to be used as the scheduler (searched recursively)
#  $2 The name of the algorithm to search for (will search for ${2}.c in $1)
# Returns 0 if the simulator is compiled without issue, or 1 if there is a failure
setup_simulator() {
  local submission="${1}"
  local algorithm="${2}"

  sim_dir=$(mktemp -d) && pushd "${sim_dir}" &> /dev/null || return 1
  cp "${simulator}" "${sim_dir}/" && find "${submission}" -name "${algorithm}".c -type f -exec cp {} "${sim_dir}"/"scheduler.c" \; || return 1
  if "${debug}"
  then
    gcc -Wall -o simulator simulator.c && [[ -x simulator ]] || return 1
  else
    gcc -Wall -o simulator simulator.c &> /dev/null && [[ -x simulator ]] || return 1
  fi
  return 0
}

# Remove a simulator set up by setup_simulator, if possible
teardown_simulator() {
  if ! [[ -z "${sim_dir-}" ]] && [[ -d "${sim_dir}" ]]
    then
      popd &> /dev/null
      rm -R "${sim_dir}" || warn "Unable to remove ${sim_dir}"
  fi
}

# Execute the simulator with the given schedule, writing output to the given output file
# Parameters:
#   $1 The schedule file to use
#   $2 Where to write the output to
# Returns 0 if the simulator completes successfully, or 1 if there is a failure
process_schedule() {
  local schedule="${1}"
  local schedule_output="${2}"

  if "${debug}"
  then
    touch "${schedule_output}" && timeout "${timeout}" ./simulator -d "${schedule}" 2>&1 | tee "${schedule_output}" || return 1
  else
    touch "${schedule_output}" && timeout "${timeout}" ./simulator "${schedule}" > "${schedule_output}" 2> /dev/null || return 1
  fi
  return 0
}

# Determines if the given schedule is different to any currently in $schedules
# Parameters:
#   $1 The schedule to test
# Returns 0 if no file in $schedules is identical to the one given in $1, or 1 if there is a matching schedule already
unique_schedule() {
  local test_schedule="${1}"

  for existing_schedule in "${schedules}"/*
  do
    if [[ -f "${existing_schedule}" ]]
    then
      if diff -q "${existing_schedule}" "${test_schedule}" &> /dev/null
      then
        return 1
      fi
    fi
  done
  return 0
}

# Collect the schedule file from the given submission, provided it is valid,
# and copy it to a randomly-named file in $schedules
# Parameters:
#   $1 The submission directory to search through
collect_schedule() {
  local submission="${1}"
  local tmp_file=$(mktemp "spec_schedule_XXXXX")

  find "${submission}" -name "spec_schedule.txt" -type f -exec cp {} "${tmp_file}" \;
  if [[ -s "${tmp_file}" ]]
  then
    dos2unix "${tmp_file}" &> /dev/null  # Ensure in correct format
    echo >> "${tmp_file}"  # Add new line

    if process_schedule "${tmp_file}" "/dev/null"
    then
      if unique_schedule "${tmp_file}"
      then
        cp "${tmp_file}" "${schedules}"/
        success "Valid spec_schedule.txt from $(basename "${submission}") copied to $(basename "${tmp_file}")"
      else
        success "Valid spec_schedule.txt from $(basename "${submission}") (not unique)"
      fi
    else
      warn "Invalid spec_schedule.txt from $(basename "${submission}")"
    fi
  else
    warn "No spec_schedule.txt from $(basename "${submission}")"
  fi

  rm "${tmp_file}" &> /dev/null
}

# Collect schedule files from a directory of submissions into $schedules
collect_schedules() {
  # only collect schedules if we have a valid fcfs implementation
  if setup_simulator "${solutions}" "fcfs"
  then
    if "${single}"
    then
      # collect the single submission, if only testing a single submission
      collect_schedule "${submissions}"
    else
      # collect the schedule from each submission, if testing multiple submissions
      for submission in "${submissions}"/*
      do
        [[ -d "${submission}" ]] && collect_schedule "${submission}"
      done
    fi
  else
    die "Unable to set up FCFS to test schedules"
  fi
  teardown_simulator
}

# Processes the given algorithm from the given submission
# Parameters:
#   $1 The directory containing the submission
#   $2 The algorithm to process
process_algorithm() {
  local submission="${1}"
  local algorithm="${2}"
  local submission_output_dir="${output}"/"${algorithm}"/$(basename "${submission}")

  info "Processing $(basename "${submission}")"
  # ensure directories/files exist
  mkdir -p "${submission_output_dir}"
  for schedule in "${schedules}"/*
  do
      touch "${submission_output_dir}"/$(basename "${schedule}")
  done

  # setup simulator and test all schedules
  if setup_simulator "${submission}" "${algorithm}"
  then
    info "\tProcessing ${algorithm}"
    for schedule in "${schedules}"/*
    do
      if process_schedule "${schedule}" "${submission_output_dir}"/$(basename "${schedule}")
      then
        success "Processed $(basename "${schedule}")"
      else
        warn "Unable to process $(basename "${schedule}")"
      fi
    done
  else
    error "Unable to set up simulator with ${algorithm} from $(basename "${submission}")"
  fi
  teardown_simulator
  msg ""
}

# Calculate marks based on the number of schedules processed correctly
# Parameters:
#   $1 The directory containing the submission to mark
#   $2 The algorithm to compute the marks for
calculate_marks() {
  local submission="${1}"
  local algorithm="${2}"

  local correct_output="${output}"/"${algorithm}"/$(basename "${solutions}")
  local submission_output="${output}"/"${algorithm}"/$(basename "${submission}")

  local num_tests=$(find "${correct_output}/" -type f | wc -l | xargs)
  local failed=$(diff -q "${correct_output}" "${submission_output}" 2>&1 | wc -l)
  local score=$(echo scale=2\;"${marks}*(${num_tests}-${failed})/${num_tests}" | bc -l)
  msg "\t$(basename "${submission}")\t${score}"
}

# Output the average turnaround time for the given submission and algorithm
# Parameters:
#   $1 The directory containing the submission to get averages for
#   $2 The algorithm to get averages for
#   $3 The file to store the average in
output_averages () {
  local submission="${1}"
  local algorithm="${2}"
  local competition_output="${3}"
  local submission_output_dir="${output}"/"${algorithm}"/$(basename "${submission}")

  for schedule in "${schedules}"/*
  do
    schedule_to_test="${submission_output_dir}"/$(basename "${schedule}")
    local val="NULL"
    if [[ -f "${schedule_to_test}" ]]
    then
      val=$((grep "Average turnaround time:" "${schedule_to_test}" | cut -f2) || echo "")
    fi
    [[ -z "${val}" ]] && val="NULL"
    echo "$(basename "${submission}"),$(basename "${schedule}"),"${val}"" >> "${competition_output}"
  done
}

# Print out configuration information
print_configuration() {
  info "Configuration:"
  info "\tdebug:       ${debug}"
  info "\tsimulator:   ${simulator}"
  info "\toutput:      ${output}"
  info "\tschedules:   ${schedules}"
  info "\tsolutions:   ${solutions}"
  info "\tsubmissions: ${submissions}"
  info "\tcollect:     ${collect}"
  info "\tgenerate:    ${generate}"
  info "\tsingle:      ${single}"
  info "\ttimeout:     ${timeout}"
  info "\tmarks:       ${marks}"
  info "\tcompetition: ${competition}"
  info "\talgorithms:  ${args[*]}"
  msg ""
}

# Main part of program
main() {
  print_configuration

  # Collect schedules
  if "${collect}"
  then
    collect_schedules
  fi

  # Generate output for submissions
  if "${generate}"
  then
    for algorithm in "${args[@]}"
    do
      if "${single}"
      then
        process_algorithm "${submissions}" ${algorithm}
      else
        for submission in "${submissions}"/*
        do
          [[ -d "${submission}" ]] && process_algorithm "${submission}" ${algorithm}
        done
      fi

      if "${generate_answers}"
      then
        # generate solution output, if possible
        process_algorithm "${solutions}" ${algorithm}
      fi
    done
  fi

  # Generate output for competition
  if "${generate}" && [[ 0 -ne "${competition}" ]]
  then
    if "${single}"
    then
      process_algorithm "${submissions}" "custom"
    else
      for submission in "${submissions}"/*
      do
        [[ -d "${submission}" ]] && process_algorithm "${submission}" "custom"
      done
    fi
 fi

  # Output scores for algorithms
  if ! "${debug}" && [[ 0 -ne "${marks}" ]]
  then
    for algorithm in "${args[@]}"
    do
      msg "\nMarks for ${algorithm}:"
      if "${single}"
      then
        calculate_marks "${submissions}" "${algorithm}"
      else
        for submission in "${submissions}"/*
        do
          [[ -d "${submission}" ]] && calculate_marks "${submission}" "${algorithm}"
        done
      fi
    done
  fi

  # Output scores for competition
  if ! "${debug}" && [[ 0 -ne "${competition}" ]]
  then
    msg ""
    info "Competition results:"
    competition_output="${output}"/"custom"/"competition_$(date -u "+%Y-%m-%d_%H-%M-%S").csv"
    echo "Submission,Schedule,Average" > "${competition_output}"
    if "${single}"
    then
      output_averages "${submissions}" "custom" "${competition_output}"
    else
      for submission in "${submissions}"/*
      do
        [[ -d "${submission}" ]] && output_averages "${submission}" "custom" "${competition_output}"
      done
    fi
    python3 score_competition.py "${competition_output}" "${competition}" | tee "${competition_output}"_results.txt
  fi
}

parse_params() {
  # default values of variables set from params
  debug=false
  simulator="simulator.c"
  schedules="schedules"
  collect=false
  solutions="algorithms"
  submissions="submissions"
  single=false
  generate=true
  generate_answers=false
  output="output"
  timeout="1m"
  marks=0
  competition=0

  while :; do
    case "${1-}" in
    -h | --help) usage && exit ;;
    -v | --verbose) set -x ;;
    --no-color) NO_COLOR=1 && setup_colors ;;
    -d | --debug) debug=true ;;
    -1 | --single) single=true ;;
    -n | --no-generate) generate=false ;;
    -a | --generate-answers) generate_answers=true ;;
    -k | --collect) collect=true ;;
    -s | --simulator)
      simulator="${2-}"
      shift
      ;;
    -e | --schedules)
      schedules="${2-}"
      shift
      ;;
    -l | --solutions)
      solutions="${2-}"
      shift
      ;;
    -u | --submissions)
      submissions="${2-}"
      shift
      ;;
    -o | --output)
      output="${2-}"
      shift
      ;;
    -t | --timeout)
      timeout="${2-}"
      shift
      ;;
    -m | --marks)
      marks="${2-}"
      shift
      ;;
    -c | --competition)
      competition="${2-}"
      shift
      ;;
    -?*) usage_die "Unknown option: $1" ;;
    *) break ;;
    esac
    shift
  done

  args=("$@")

  # check required params and arguments

  # create required output directories
  mkdir -p "${output}"
  mkdir -p "${schedules}"

  # ensure required files/locations exist
  simulator=$(realpath "${simulator}")
  schedules=$(realpath "${schedules}")
  solutions=$(realpath "${solutions}")
  submissions=$(realpath "${submissions}")
  output=$(realpath "${output}")

  # ensure simulator is a regular file
  [[ -f "${simulator}" ]] || usage_die "simulator must be a regular tgz file (${simulator} is not)"

  # ensure marks and competition are an integers
  [[ "${marks}" =~ ^[0-9]+$ ]] || usage_die "marks must be an integer (${marks} is not)"
  [[ "${competition}" =~ ^[0-9]+$ ]] || usage_die "competition must be an integer (${competition} is not)"

  # ensure either competition or at least one algorithm are specified
  if [[ 0 -eq "${competition}" ]]
  then
    [[ ${#args[@]} -eq 0 ]] && usage_die "At least one algorithm or the competition is required"
  fi

  return 0
}

cleanup() {
  trap - SIGINT SIGTERM ERR EXIT
  teardown_simulator
}

setup_colors() {
  if [[ -t 2 ]] && [[ -z "${NO_COLOR-}" ]] && [[ "${TERM-}" != "dumb" ]]; then
    NOFORMAT='\033[0m' RED='\033[0;31m' GREEN='\033[0;32m' ORANGE='\033[0;33m' BLUE='\033[0;34m' PURPLE='\033[0;35m' CYAN='\033[0;36m' YELLOW='\033[1;33m'
  else
    NOFORMAT='' RED='' GREEN='' ORANGE='' BLUE='' PURPLE='' CYAN='' YELLOW=''
  fi
}

msg() {
  echo >&2 -e "${1-}"
}

success() {
  msg "${GREEN}Success:${NOFORMAT} ${1}"
}

info() {
  msg "${BLUE}Info:${NOFORMAT} ${1}"
}

warn() {
  msg "${ORANGE}Warning:${NOFORMAT} ${1}"
}

error() {
  msg "${RED}Error:${NOFORMAT} ${1}"
}

die() {
  local msg=$1
  local code=${2-1}  # default exit status 1
  error "$msg\n"
  exit "$code"
}

usage_die() {
  usage
  die "$@"
}

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" &>/dev/null && pwd -P)

setup_colors
parse_params "$@"
main "$@"


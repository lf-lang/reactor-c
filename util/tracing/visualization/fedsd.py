#!/usr/bin/env python3

# Utility that reports the interactions (exchanged messages) between federates and the RTI in a 
# sequence-diagram-like format, or between enclaves in an enclaved execution.
# 
# The utility operates on lft trace files and outputs an HTML file with a sticky
# header, or a standalone SVG file if -v/--svg is given.

'''
In the dataframe, each row will be marked with one op these values:
    - 'arrow': draw a solid arrow
    - 'dot': draw a dot only
    - 'marked': marked, not to be drawn
    - 'pending': pending
    - 'adv': for reporting logical time advancing, draw a simple dash
'''

# Styles to determine appearance:
css_style = ' <style> \
    line { \
        stroke: black; \
        stroke-width: 2; \
    } \
    .ABS        { stroke: #d9dd1f; fill: #d9dd1f; } \
    .LTC        { stroke: #073b4c; fill: #073b4c; } \
    .T_MSG      { stroke: #ef476f; fill: #ef476f; } \
    .P2P_T_MSG  { stroke: #c77dff; fill: #c77dff; } \
    .NET        { stroke: #118ab2; fill: #118ab2; } \
    .PTAG       { stroke: #06d6a0; fill: #06d6a0; } \
    .TAG        { stroke: #08a578; fill: #08a578; } \
    .DNET       { stroke: #7b2d8b; fill: #7b2d8b; } \
    .TIMESTAMP  { stroke: #888888; fill: #888888; } \
    .FED_ID     { stroke: #80DD99; fill: #80DD99; } \
    .UPSTREAM_CONNECTED { stroke: #f4a261; fill: #f4a261; } \
    .UPSTREAM_DISCONNECTED { stroke: #e76f51; fill: #e76f51; } \
    .DOWNSTREAM_CONNECTED { stroke: #2a9d8f; fill: #2a9d8f; } \
    .DOWNSTREAM_DISCONNECTED { stroke: #264653; fill: #264653; } \
    .ACK        { stroke: #52b788; fill: #52b788; } \
    .FAILED     { stroke: #c1121f; fill: #c1121f; } \
    .STOP       {stroke: #d0b7eb; fill: #d0b7eb} \
    .STOP_REQ   { stroke: #e76f51; fill: #e76f51; } \
    .STOP_REQ_REP { stroke: #ca6702; fill: #ca6702; } \
    .STOP_GRN   { stroke: #e9c46a; fill: #e9c46a; } \
    .REJECT     { stroke: #9b2226; fill: #9b2226; } \
    .RESIGN     { stroke: #6d4c41; fill: #6d4c41; } \
    .CLOSE_RQ   { stroke: #7f7f7f; fill: #7f7f7f; } \
    .MSG        { stroke: #00b4d8; fill: #00b4d8; } \
    .P2P_MSG    { stroke: #0077b6; fill: #0077b6; } \
    .ADR_AD     { stroke: #80b918; fill: #80b918; } \
    .ADR_QR     { stroke: #48cae4; fill: #48cae4; } \
    .ADR_QR_REP { stroke: #0096c7; fill: #0096c7; } \
    .UNIDENTIFIED { stroke: #adb5bd; fill: #adb5bd; } \
    .ADV        { stroke: #e63946; fill: #e63946; stroke-linecap: round; } \
    text { \
        font-size: smaller; \
        font-family: sans-serif; \
    } \
    text.time {fill: #074936; } \
</style> \
'

# Disctionary for pruning event names. Usefule for tracepoint matching and
# communication rendering
prune_event_name = {
    "Sending ACK": "ACK",
    "Sending FAILED": "FAILED",
    "Sending TIMESTAMP": "TIMESTAMP",
    "Sending NET": "NET",
    "Sending LTC": "LTC",
    "Sending STOP_REQ": "STOP_REQ",
    "Sending STOP_REQ_REP": "STOP_REQ_REP",
    "Sending STOP_GRN": "STOP_GRN",
    "Sending FED_ID": "FED_ID",
    "Sending UPSTREAM_CONNECTED": "UPSTREAM_CONNECTED",
    "Sending UPSTREAM_DISCONNECTED": "UPSTREAM_DISCONNECTED",
    "Sending DOWNSTREAM_CONNECTED": "DOWNSTREAM_CONNECTED",
    "Sending DOWNSTREAM_DISCONNECTED": "DOWNSTREAM_DISCONNECTED",
    "Sending PTAG": "PTAG",
    "Sending TAG": "TAG",
    "Sending REJECT": "REJECT",
    "Sending RESIGN": "RESIGN",
    "Sending PORT_ABS": "ABS",
    "Sending CLOSE_RQ": "CLOSE_RQ",
    "Sending TAGGED_MSG": "T_MSG",
    "Sending P2P_TAGGED_MSG": "P2P_T_MSG",
    "Sending MSG": "MSG",
    "Sending P2P_MSG": "P2P_MSG",
    "Sending ADR_AD": "ADR_AD",
    "Sending ADR_QR": "ADR_QR",
    "Sending ADR_QR_REP": "ADR_QR_REP",
    "Sending DNET": "DNET",
    "Receiving ACK": "ACK",
    "Receiving FAILED": "FAILED",
    "Receiving TIMESTAMP": "TIMESTAMP",
    "Receiving NET": "NET",
    "Receiving LTC": "LTC",
    "Receiving STOP_REQ": "STOP_REQ",
    "Receiving STOP_REQ_REP": "STOP_REQ_REP",
    "Receiving STOP_GRN": "STOP_GRN",
    "Receiving FED_ID": "FED_ID",
    "Receiving UPSTREAM_CONNECTED": "UPSTREAM_CONNECTED",
    "Receiving UPSTREAM_DISCONNECTED": "UPSTREAM_DISCONNECTED",
    "Receiving DOWNSTREAM_CONNECTED": "DOWNSTREAM_CONNECTED",
    "Receiving DOWNSTREAM_DISCONNECTED": "DOWNSTREAM_DISCONNECTED",
    "Receiving PTAG": "PTAG",
    "Receiving TAG": "TAG",
    "Receiving REJECT": "REJECT",
    "Receiving RESIGN": "RESIGN",
    "Receiving PORT_ABS": "ABS",
    "Receiving CLOSE_RQ": "CLOSE_RQ",
    "Receiving TAGGED_MSG": "T_MSG",
    "Receiving P2P_TAGGED_MSG": "P2P_T_MSG",
    "Receiving MSG": "MSG",
    "Receiving P2P_MSG": "P2P_MSG",
    "Receiving ADR_AD": "ADR_AD",
    "Receiving ADR_QR": "ADR_QR",
    "Receiving ADR_QR_REP": "ADR_QR_REP",
    "Receiving DNET": "DNET",
    "Receiving UNIDENTIFIED": "UNIDENTIFIED",
    "Scheduler advancing time ends": "AdvLT",
    "Sending STOP": "STOP",
    "Receiving STOP": "STOP"
}

prune_event_name.setdefault(" ", "UNIDENTIFIED")

import argparse         # For arguments parsing
import pandas as pd     # For csv manipulation
import os
import re
import sys
from pathlib import Path
import math
import subprocess


def format_actor_name(name):
    '''
    Format an actor name for display inside a rectangle.
    - "RTI" is kept as-is.
    - Compiler-generated federate names of the form "federate__<base>_main_<id>"
      are shortened to "<base>: <id>" (e.g. "federate__up1_main_0" -> "up1: 0").
    - Any other name is returned unchanged.
    '''
    if name == 'RTI':
        return 'RTI'
    m = re.match(r'^federate__(.+)_main_(\d+)$', name)
    if m:
        return m.group(1) + ': ' + m.group(2)
    return name

# Nanoseconds per unit, for compact tag labels (seconds is the coarsest unit used).
_USEC = 1000
_MSEC = 1000000
_SEC = 1000000000
# UINT_MAX as written by trace_to_csv's %d, and the unsigned value itself.
_MICROSTEP_MINUS_ONE = frozenset((-1, 0xFFFFFFFF))

def format_time_value(time_ns):
    '''
    Format an elapsed time in nanoseconds using the coarsest unit among s, ms, and us
    that divides the value evenly. Otherwise show nanoseconds.
    Zero is shown as "0" with no unit.
    '''
    time_ns = int(time_ns)
    sign = ''
    if time_ns < 0:
        sign = '-'
        time_ns = -time_ns
    if time_ns == 0:
        return '0'
    if time_ns % _SEC == 0:
        return sign + str(time_ns // _SEC) + 's'
    if time_ns % _MSEC == 0:
        return sign + str(time_ns // _MSEC) + 'ms'
    if time_ns % _USEC == 0:
        return sign + str(time_ns // _USEC) + 'us'
    return sign + f'{time_ns:,}ns'

def format_tag(logical_time, microstep):
    '''
    Format a (logical_time, microstep) pair for a signal label.

    A microstep of -1 (UINT_MAX printed as a signed int) is one microstep
    earlier than (logical_time + 1 nsec, 0). Display that as (next_time, -1),
    e.g. (9,999,999 ns, -1) becomes "(10ms, -1)".
    '''
    time_ns = int(logical_time)
    step = int(microstep)
    if step in _MICROSTEP_MINUS_ONE:
        time_ns += 1
        step = -1
    return format_time_value(time_ns) + ', ' + str(step)

def layout_actors(actors, padding, spacing, center_rti=False):
    '''
    Place actors left to right and return (ordered_actors, x_coor, extra_left).
    If center_rti is True and there is more than one federate, the RTI (id -1)
    is inserted in the middle of the federate list instead of at the left.
    extra_left is additional left margin so AdvLT labels on the leftmost
    federate are not clipped by the SVG viewport.
    '''
    ordered = list(actors)
    extra_left = 0
    if center_rti and -1 in ordered and len(ordered) > 2:
        federates_only = [a for a in ordered if a != -1]
        mid = len(federates_only) // 2
        ordered = federates_only[:mid] + [-1] + federates_only[mid:]
        # Match the extra space already reserved on the right for labels.
        extra_left = 200
    x_coor = {}
    for i, actor_id in enumerate(ordered):
        x_coor[actor_id] = extra_left + (padding * 2) + (spacing * i)
    return ordered, x_coor, extra_left

# Define the arguments to pass in the command line
parser = argparse.ArgumentParser(description='Set of the lft trace files to render.')
parser.add_argument('-r','--rti', type=str, 
                    help='RTI\'s lft trace file.')
parser.add_argument('-f','--federates', nargs='+',
                    help='List of the federates\' lft trace files.')
parser.add_argument('-s', '--start', type=str, nargs=2, metavar=('TIME', 'UNIT'),
                    help='Start time of visualization in elapsed logical time, e.g. 0 ms. '
                         'Units: ns, us, ms, s, min, hour, day, week (or nsec, usec, msec, sec, ...).')
parser.add_argument('-e', '--end', type=str, nargs=2, metavar=('TIME', 'UNIT'),
                    help='End time of visualization in elapsed logical time, e.g. 20 ms. Same units as -s.')
parser.add_argument('-v', '--svg', action='store_true',
                    help='Generate a pure SVG file (trace_svg.svg) instead of HTML (trace_svg.html).')
parser.add_argument('-np', '--no-physical-times', action='store_true',
                    help='Omit physical time labels from the sequence diagram.')
parser.add_argument('-c', '--center-rti', action='store_true',
                    help='Place the RTI in the middle of the diagram instead of on the left.')

# Events matching at the sender and receiver ends depend on whether they are tagged
# (the elapsed logical time and microstep have to be the same) or not. 
# Set of non-tagged events (messages)
non_tagged_messages = {'FED_ID', 'UPSTREAM_CONNECTED', 'UPSTREAM_DISCONNECTED', 'DOWNSTREAM_CONNECTED', 'DOWNSTREAM_DISCONNECTED',
                       'ACK', 'RESIGN', 'FAILED', 'REJECT', 'ADR_QR', 'ADR_QR_REP', 'ADR_AD', 'MSG', 'P2P_MSG', 'STOP'}


################################################################################
### Routines to get svg descriptions
################################################################################

def svg_string_draw_line(x1, y1, x2, y2, type=''):
    '''
    Constructs the svg html string to draw a line from (x1, y1) to (x2, y2).

    Args:
     * x1: Int X coordinate of the source point
     * y1: Int Y coordinate of the source point
     * x2: Int X coordinate of the sink point
     * y2: Int Y coordinate of the sink point
     * type: The type of the message (for styling)
    Returns:
     * String: the svg string of the line
    '''
    str_line = '\t<line x1="'+str(x1)+'" y1="'+str(y1)+'" x2="'+str(x2)+'" y2="'+str(y2)+'"'
    if (type):
            str_line = str_line + ' class="' + type + '"'
 
    str_line = str_line +  '/>\n'
    return str_line


def svg_string_draw_arrow_head(x1, y1, x2, y2, type='') :
    '''
    Constructs the svg html string to draw the arrow end

    Args:
     * x1: Int X coordinate of the source point
     * y1: Int Y coordinate of the source point
     * x2: Int X coordinate of the sink point
     * y2: Int Y coordinate of the sink point
     * type: The type (for styling)
    Returns:
     * String: the svg string of the triangle
    '''

    if (y2 != y1):
        rotation = - math.ceil(math.atan((x2-x1)/(y2-y1)) * 180 / 3.14) - 90
    else:
        if (x1 > x2):
            rotation = 0
        else:
            rotation = - 180
        
    style = ''
    if (type):
        style = ' class="'+type+'"'
    
    str_line = ''
    if (x1 > x2) :
        # SVG rotate(angle, cx, cy) is portable; CSS transform-origin is not
        # (Adobe Illustrator ignores it and rotates around the document origin).
        str_line = '\t<path d="M'+str(x2)+' '+str(y2)+' L'+str(x2+10)+' '+str(y2+5)+' L'+str(x2+10)+' '+str(y2-5)+' Z"' \
             + ' transform="rotate('+str(rotation)+' '+str(x2)+' '+str(y2)+')"' \
             + style \
             + '/>\n'
    else :
        str_line = '\t<path d="M'+str(x2)+' '+str(y2)+' L'+str(x2-10)+' '+str(y2+5)+' L'+str(x2-10)+' '+str(y2-5)+' Z"' \
             + ' transform="rotate('+str( 180 + rotation)+' '+str(x2)+' '+str(y2)+')"' \
             + style \
             + '/>\n'

    return str_line


def svg_string_draw_label(x1, y1, x2, y2, label) :
    '''
    Computes the rotation angle of the text and then constructs the svg string. 

    Args:
     * x1: Int X coordinate of the source point
     * y1: Int Y coordinate of the source point
     * x2: Int X coordinate of the sink point
     * y2: Int Y coordinate of the sink point
     * label: The label to draw
    Returns:
     * String: the svg string of the text
    '''
    # FIXME: Need further improvement, based of the position of the arrows
    # FIXME: Rotation value is not that accurate. 
    if (x2 < x1) :
        # Left-going arrow.
        if (y2 != y1):
            rotation = - math.ceil(math.atan((x2-x1)/(y2-y1)) * 180 / 3.14) - 90
        else:
            rotation = 0

        str_line = '\t<text text-anchor="end" transform="translate('+str(x1-10)+', '+str(y1-5)+') rotate('+str(rotation)+')">'+label+'</text>\n'
    else :
        # Right-going arrow.
        if (y2 != y1):
            rotation = - math.ceil(math.atan((x1-x2)/(y1-y2)) * 180 / 3.14) + 90
        else:
            rotation = 0
        str_line = '\t<text transform="translate('+str(x1+10)+', '+str(y1-5)+') rotate('+str(rotation)+')" text-anchor="start">'+label+'</text>\n'
    return str_line


def svg_string_draw_arrow(x1, y1, x2, y2, label, type=''):
    '''
    Constructs the svg html string to draw the arrow from (x1, y1) to (x2, y2). 
    The arrow end is constructed, together with the label

    Args:
     * x1: Int X coordinate of the source point
     * y1: Int Y coordinate of the source point
     * x2: Int X coordinate of the sink point
     * y2: Int Y coordinate of the sink point
     * label: String Label to draw on top of the arrow
     * type: The type of the message
    Returns:
     * String: the svg string of the arrow
    '''
    str_line1 = svg_string_draw_line(x1, y1, x2, y2, type)
    str_line2 = svg_string_draw_arrow_head(x1, y1, x2, y2, type)
    str_line3 = svg_string_draw_label(x1, y1, x2, y2, label)
    return str_line1 + str_line2 + str_line3

def svg_string_draw_side_label(x, y, label, anchor="start") :
    '''
    Put a label to the right of the x, y point,
    unless x is small, in which case put it to the left.

    Args:
     * x: Int X coordinate of the source point
     * y: Int Y coordinate of the source point
     * label: Label to put by the point.
     * anchor: One of "start", "middle", or "end" to specify the text-anchor.
    Returns:
     * String: the svg string of the text
    '''
    offset = 5
    if (anchor == 'end'):
        offset = -5
    elif (anchor == 'middle'):
        offset = 0
    str_line = '\t<text text-anchor="'+anchor+'"' \
    +' class="time"' \
    +' transform="translate('+str(x+offset)+', '+str(y+5)+')">'+label+'</text>\n'
    return str_line

def svg_string_comment(comment):
    '''
    Constructs the svg html string to write a comment into an svg file.

    Args:
     * comment: String Comment to add
    Returns:
     * String: the svg string of the comment
    '''
    str_line = '\n\t<!-- ' + comment + ' -->\n'
    return str_line

def svg_string_draw_dot(x, y, label) :
    '''
    Constructs the svg html string to draw at a dot.

    Args:
     * x: Int X coordinate of the dot
     * y: Int Y coordinate of the dot
     * label: String to draw 
    Returns:
     * String: the svg string of the triangle
    '''
    str_line = ''
    str_line = '\t<circle cx="'+str(x)+'" cy="'+str(y)+'" r="3" stroke="black" stroke-width="1" fill="black"/>\n'
    str_line = str_line + '\t<text x="'+str(x+5)+'", y="'+str(y+5)+'" fill="blue">'+label+'</text>\n'
    return str_line

def svg_string_draw_dot_with_time(x, y, time, label) :
    '''
    Constructs the svg html string to draw at a dot with a prefixed physical time.

    Args:
     * x: Int X coordinate of the dot
     * y: Int Y coordinate of the dot
     * time: The time
     * label: String to draw 
    Returns:
     * String: the svg string of the triangle
    '''
    str_line = ''
    str_line = '\t<circle cx="'+str(x)+'" cy="'+str(y)+'" r="3" stroke="black" stroke-width="1" fill="black"/>\n'
    str_line = str_line + '\t<text x="'+str(x+5)+'", y="'+str(y+5)+'"> <tspan class="time">'+time+':</tspan> <tspan fill="blue">'+label+'</tspan></text>\n'
    return str_line

def svg_string_draw_adv(x, y, label, anchor="start") :
    '''
    Constructs the svg html string to draw at a dash, meaning that logical time is advancing there.

    Args:
     * x: Int X coordinate of the dash
     * y: Int Y coordinate of the dash
     * label: String to draw
     * anchor: "start" puts the label to the right of the line, "end" to the left.
    Returns:
     * String: the svg string of the triangle
    '''
    str_line1 = svg_string_draw_line(x-5, y, x+5, y, "ADV")
    str_line2 = svg_string_draw_side_label(x, y, label, anchor)
    return str_line1 + str_line2


################################################################################
### Routines to process lft and csv files
################################################################################

def load_and_process_csv_file(csv_file) :
    '''
    Loads and processes the csv entries, based on the type of the actor (if RTI
    or federate).

    Args:
     * csv_file: String file name
    Returns:
     * The processed dataframe.
    '''
    # Load tracepoints, rename the columns and clean non useful data
    df = pd.read_csv(csv_file)
    df.columns = ['event', 'reactor', 'self_id', 'partner_id', 'logical_time', 'microstep', 'physical_time', 't', 'ed']
    df = df.drop(columns=['reactor', 't', 'ed'])

    # Remove all the lines that do not contain communication information
    # which boils up to having 'RTI' in the 'event' column
    df = df[df['event'].str.contains('Sending|Receiving|Scheduler advancing time ends') == True]

    # Determine the "self id" in the trace file based on the first 'Receiving' or 'Sending' message (or use -1, the id of the RTI, if there is none).
    id = -1
    for index, row in df.iterrows():
        if ('Sending' in row['event'] or 'Receiving' in row['event']) :
            id = row['self_id']
            break
    df['self_id'] = id
    df = df.astype({'self_id': 'int', 'partner_id': 'int'})

    # Add an inout column to set the arrow direction
    df['inout'] = df['event'].apply(lambda e: 'in' if 'Receiving' in e else 'out')

    # Prune event names
    df['event'] = df['event'].apply(lambda e: prune_event_name[e])
    return df

def command_is_in_path(command):
    '''
    Checks if a command is in the PATH.

    Args:
     * command: The command to check.
    Returns:
     * True if the command is in the PATH, False otherwise.
    '''
    # Get the PATH environment variable.
    path = os.environ["PATH"]

    # Split the PATH into a list of directories.
    directories = path.split(os.pathsep)

    # Check if the command is in the list of directories.
    for directory in directories:
        if os.path.isdir(directory):
            if command in os.listdir(directory):
                return True
    return False

def convert_lft_file_to_csv(lft_file, start_time, end_time):
    '''
    Call trace_to_csv command to convert the given binary lft trace file to csv format.

    Args:
     * lft_file: the lft trace file
    Return:
     * File: the converted csv file, if the conversion succeeds, and empty string otherwise.
     * String: the error message, in case the conversion did not succeed, and empty string otherwise.
    '''

    subprocess_args = ['trace_to_csv', lft_file]
    if (start_time != None):
        subprocess_args.extend(['-s', start_time[0], start_time[1]])
    if (end_time != None):
        subprocess_args.extend(['-e', end_time[0], end_time[1]])

    convert_process = subprocess.run(subprocess_args, capture_output=True, text=True)

    if (convert_process.returncode == 0):
        csv_file = os.path.splitext(lft_file)[0] + '.csv'
        return csv_file, ''
    else:
        error = (convert_process.stderr or convert_process.stdout or '').strip()
        if not error:
            error = 'trace_to_csv exited with code ' + str(convert_process.returncode)
        return '', error

def get_and_convert_lft_files(rti_lft_file, federates_lft_files, start_time, end_time):
    '''
    Check if the passed arguments are valid, in the sense that the files do exist.
    If not arguments were passed, then look up the local lft files.
    Then, convert to csv.

    Args:
     * File: the argument passed at the command line as the rti lft trace file.
     * Array: the argument passed at the command line as array of federates lft trace files.
    Return:
     * File: the converted RTI trace csv file, or empty, if no RTI trace lft file is found
     * Array: Array of files of converted federates trace csv files
    '''
    if (not rti_lft_file and not federates_lft_files):
        federates_lft_files = []
        
        for file in os.listdir():
            if (file == 'rti.lft'):
                rti_lft_file = 'rti.lft'
            elif (file.endswith('.lft')):
                federates_lft_files.append(file)
    else:
        # If files were given, then check they exist
        if (rti_lft_file):
            if (not os.path.exists(rti_lft_file)):
                print('Warning: Trace file ' + rti_lft_file + ' does not exist! Will resume though')
        else: 
            for file in federates_lft_files:
                if (not os.path.exists(file)):
                    print('Warning: Trace file ' + file + ' does not exist! Will resume though')

    # Sanity check that there is at least one lft file!
    if (not rti_lft_file and not federates_lft_files):
        print('Fedsd: Error: No lft files found. Abort!')
        sys.exit(1)

    # Now, convert lft files to csv
    rti_csv_file = ''
    if (rti_lft_file):
        rti_csv_file, error = convert_lft_file_to_csv(rti_lft_file, start_time, end_time)
        if (not rti_csv_file):
            print('Fedsd: Error converting the RTI\'s lft file: ' + error)
        else:
            print('Fedsd: Successfully converted trace file ' + rti_lft_file + ' to ' + rti_csv_file + '.')
    
    federates_csv_files = []
    for file in federates_lft_files:
        fed_csv_file, error = convert_lft_file_to_csv(file, start_time, end_time)
        if (not fed_csv_file):
            print('Fedsd: Error converting the federate lft file ' + file + ': ' + error)
        else: 
            print('Fedsd: Successfully converted trace file ' + file + ' to ' + fed_csv_file + '.')
            federates_csv_files.append(fed_csv_file)
        
    return rti_csv_file, federates_csv_files

################################################################################
### Routines to write the sequence diagram
################################################################################

def write_actor_headers(f, x_coor, actors_names, padding):
    '''
    Write actor rectangles and labels (the sequence-diagram "lifeline" headers).

    Args:
     * f: Open file handle to write to
     * x_coor: Dict mapping actor id to X coordinate
     * actors_names: Dict mapping actor id to name
     * padding: Int padding used to size and place the headers
    '''
    for key in x_coor:
        title = format_actor_name(actors_names[key])
        cx = x_coor[key]
        cy = math.ceil(padding / 2)
        r = 20  # original circle radius; diameter becomes the rect height
        rect_w = max(r * 2, len(title) * 7 + 12)
        rect_h = r * 2
        # Draw rectangle then bold text centered on it (later = on top in SVG)
        f.write('\t<rect x="'+str(cx - rect_w//2)+'" y="'+str(cy - rect_h//2)+'" '
                +'width="'+str(rect_w)+'" height="'+str(rect_h)+'" fill="white" stroke="black" stroke-width="2"/>\n')
        f.write('\t<text x="'+str(cx)+'" y="'+str(cy)+'" text-anchor="middle" dominant-baseline="central" '
                +'font-weight="bold" fill="black">'+title+'</text>\n')


def write_diagram_body(f, x_coor, actors_names, trace_df, svg_height, show_physical_times=True):
    '''
    Write vertical actor lines and interaction arrows.

    Args:
     * f: Open file handle to write to
     * x_coor: Dict mapping actor id to X coordinate
     * actors_names: Dict mapping actor id to name
     * trace_df: Dataframe of matched trace events
     * svg_height: Int height of the diagram (not including the header)
     * show_physical_times: If False, omit physical time labels
    '''
    # Draw vertical lines for each actor (full diagram height)
    for key in x_coor:
        title = actors_names[key]
        if (key == -1):
            f.write(svg_string_comment('RTI Actor line'))
        else:
            f.write(svg_string_comment('Federate '+str(key)+': ' + title + ' Actor line'))
        f.write(svg_string_draw_line(x_coor[key], 0, x_coor[key], svg_height, False))

    # Draw interactions
    f.write(svg_string_comment('Draw interactions'))
    for index, row in trace_df.iterrows():
        # formatted physical time.
        # FIXME: Using microseconds is hardwired here.
        physical_time = f'{int(row["physical_time"]) // 1000:,}us'

        if (row['event'] in non_tagged_messages):
            label = row['event']
        else:
            label = row['event'] + '(' + format_tag(row['logical_time'], row['microstep']) + ')'

        if (row['arrow'] == 'arrow'):
            f.write(svg_string_draw_arrow(row['x1'], row['y1'], row['x2'], row['y2'], label, row['event']))
            if show_physical_times:
                if (row['inout'] in 'in'):
                    # Label at receiver (x2): goes outward — right if receiver is right of sender.
                    anchor = 'start' if row['x2'] > row['x1'] else 'end'
                    f.write(svg_string_draw_side_label(row['x2'], row['y2'], physical_time, anchor))
                else:
                    # Label at sender (x1): goes outward — left if receiver is right of sender.
                    anchor = 'end' if row['x2'] > row['x1'] else 'start'
                    f.write(svg_string_draw_side_label(row['x1'], row['y1'], physical_time, anchor))
        elif (row['arrow'] == 'dot'):
            if (row['inout'] == 'in'):
                label = "(in) from " + str(row['partner_id']) + ' ' + label
            else:
                label = "(out) to " + str(row['partner_id']) + ' ' + label

            if (row['self_id'] < 0):
                if show_physical_times:
                    f.write(svg_string_draw_side_label(row['x1'], row['y1'], physical_time, 'end'))
                f.write(svg_string_draw_dot(row['x1'], row['y1'], label))
            elif show_physical_times:
                f.write(svg_string_draw_dot_with_time(row['x1'], row['y1'], physical_time, label))
            else:
                f.write(svg_string_draw_dot(row['x1'], row['y1'], label))

        elif (row['arrow'] == 'marked'):
            if show_physical_times:
                # Label goes outward: right of receiver if receiver is right of sender, left otherwise.
                partner_x = x_coor.get(int(row['partner_id']), row['x1'])
                marked_anchor = 'start' if row['x1'] > partner_x else 'end'
                f.write(svg_string_draw_side_label(row['x1'], row['y1'], physical_time, marked_anchor))

        elif (row['arrow'] == 'adv'):
            # When the RTI is centered, put AdvLT labels on the left for federates
            # that sit to the left of the RTI so they do not overlap the RTI column.
            rti_x = x_coor.get(-1)
            adv_anchor = 'end' if (rti_x is not None and row['x1'] < rti_x) else 'start'
            f.write(svg_string_draw_adv(row['x1'], row['y1'], label, adv_anchor))


def write_html_file(svg_width, svg_height, header_height, padding, x_coor, actors_names, trace_df,
                    show_physical_times=True):
    '''
    Write the sequence diagram as an HTML file with a sticky header and an embedded SVG.
    '''
    with open('trace_svg.html', 'w', encoding='utf-8') as f:
        f.write('<!DOCTYPE html>\n<html>\n<head>\n<meta charset="UTF-8">\n')
        f.write('<style>\n')
        f.write('  * { margin: 0; padding: 0; }\n')
        # Allow horizontal scroll on the whole page when the diagram is wider than the window.
        f.write('  body { overflow-x: auto; }\n')
        # The sticky header div stays at the top of the viewport while the diagram scrolls.
        f.write('  #sticky-header { position: sticky; top: 0; z-index: 100; display: block; }\n')
        f.write('  svg { display: block; }\n')
        f.write('</style>\n')
        f.write('</head>\n<body>\n\n')

        # ---- Sticky header: circles and actor labels only ----
        f.write('<div id="sticky-header">\n')
        f.write('<svg width="'+str(svg_width)+'" height="'+str(header_height)+'">\n')
        f.write(css_style)
        # White background so it occludes the diagram lines that scroll beneath it.
        f.write('\t<rect x="0" y="0" width="'+str(svg_width)+'" height="'+str(header_height)+'" fill="white"/>\n')
        write_actor_headers(f, x_coor, actors_names, padding)
        f.write('</svg>\n')
        f.write('</div>\n\n')

        # ---- Main diagram SVG: vertical lines and all interactions ----
        f.write('<svg width="'+str(svg_width)+'" height="'+str(svg_height)+'">\n')
        f.write(css_style)
        write_diagram_body(f, x_coor, actors_names, trace_df, svg_height, show_physical_times)
        f.write('\n</svg>\n\n')
        f.write('</body>\n</html>\n')


def write_svg_file(svg_width, svg_height, header_height, padding, x_coor, actors_names, trace_df,
                   show_physical_times=True):
    '''
    Write the sequence diagram as a standalone SVG file.
    Actor headers and the diagram body are combined into a single SVG.
    '''
    total_height = header_height + svg_height
    with open('trace_svg.svg', 'w', encoding='utf-8') as f:
        f.write('<?xml version="1.0" encoding="UTF-8"?>\n')
        f.write('<svg xmlns="http://www.w3.org/2000/svg" width="'+str(svg_width)+'" height="'+str(total_height)+'">\n')
        f.write(css_style)
        f.write('\t<rect x="0" y="0" width="'+str(svg_width)+'" height="'+str(total_height)+'" fill="white"/>\n')
        write_actor_headers(f, x_coor, actors_names, padding)
        f.write('<g transform="translate(0, '+str(header_height)+')">\n')
        write_diagram_body(f, x_coor, actors_names, trace_df, svg_height, show_physical_times)
        f.write('</g>\n')
        f.write('</svg>\n')


################################################################################
### Main program to run
################################################################################

if __name__ == '__main__':
    args = parser.parse_args()

    # Check that trace_to_csv is in PATH
    if (not command_is_in_path('trace_to_csv')):
        print('Fedsd: Error: trace_to_csv utility is not in PATH. Abort!')
        sys.exit(1)

    # Look up the lft files and transform them to csv files

    rti_csv_file, federates_csv_files = get_and_convert_lft_files(args.rti, args.federates, args.start, args.end)

    if (not rti_csv_file and not federates_csv_files):
        print('Fedsd: Error: Failed to convert any lft files. Abort!')
        sys.exit(1)
    
    # The RTI and each of the federates have a fixed x coordinate. They will be
    # saved in a dict
    actors = []
    actors_names = {}
    padding = 50
    spacing = 200       # Spacing between federates

    actors.append(-1)
    actors_names[-1] = "RTI"
   
    trace_df = pd.DataFrame()

    ############################################################################
    #### Federates trace processing
    ############################################################################
    # Loop over the given list of federates trace files 
    if (federates_csv_files) :
        for fed_trace in federates_csv_files:
            try:
                fed_df = load_and_process_csv_file(fed_trace)
            except Exception as e:
                print(f"Warning: Problem processing trace file {fed_trace}: `{e}`")
                continue

            if (not fed_df.empty):
                # Get the federate id number
                fed_id = fed_df.iloc[-1]['self_id']

                ### Check that the federate id has not been entered yet.
                ### This is particularly useful for transient actors, when
                if (actors.count(fed_id) == 0): 
                    # Add to the list of sequence diagram actors and add the name
                    actors.append(fed_id)
                    actors_names[fed_id] = Path(fed_trace).stem

                trace_df = pd.concat([trace_df, fed_df])
                fed_df = fed_df[0:0]
    
    actors, x_coor, extra_left = layout_actors(actors, padding, spacing, center_rti=args.center_rti)
    if not trace_df.empty:
        trace_df['x1'] = trace_df['self_id'].apply(lambda e: x_coor[int(e)])
        
    ############################################################################
    #### RTI trace processing, if any
    ############################################################################
    if (rti_csv_file):
        rti_df = load_and_process_csv_file(rti_csv_file)
        rti_df['x1'] = x_coor[-1]
    elif (not trace_df.empty):
        # If there is no RTI, derive one.
        # This is particularly useful for tracing enclaves
        # FIXME: Currently, `fedsd` is used either for federates OR enclaves.
        # As soon as there is a consensus on how to visualize federations where
        # a federate has several enclves, the utility will be updated.
        rti_df = trace_df[['event', 'self_id', 'partner_id', 'logical_time', 'microstep', 'physical_time', 'inout']].copy()
        rti_df = rti_df[rti_df['event'].str.contains('AdvLT') == False]
        rti_df.columns = ['event', 'partner_id', 'self_id', 'logical_time', 'microstep', 'physical_time', 'inout']
        rti_df['inout'] = rti_df['inout'].apply(lambda e: 'in' if 'out' in e else 'out')
        rti_df['x1'] = rti_df['self_id'].apply(lambda e: x_coor[int(e)])
    else:
        print('Fedsd: Error: No trace data to visualize. Abort!')
        sys.exit(1)

    trace_df = pd.concat([trace_df, rti_df])

    if trace_df.empty:
        print('Fedsd: Error: No trace data to visualize. Abort!')
        sys.exit(1)

    # Sort all traces by physical time and then reset the index
    trace_df = trace_df.sort_values(by=['physical_time'])
    trace_df = trace_df.reset_index(drop=True)

    # Add the Y column and initialize it with the padding value 
    trace_df['y1'] = math.ceil(padding * 3 / 2) # Or set a small shift

    ############################################################################
    #### Compute the 'y1' coordinates
    ############################################################################
    ppt = 0     # Previous physical time
    cpt = 0     # Current physical time
    py = 0      # Previous y
    min = 15    # Minimum spacing between events when time has not advanced.
    scale = 1   # Will probably be set manually
    first_pass = True
    for index, row in trace_df.iterrows():
        if (not first_pass) :
            cpt = row['physical_time']
            # print('cpt = '+str(cpt)+' and ppt = '+str(ppt))
            # From the email:
            # Y = T_previous + min + log10(1 + (T - T_previous)*scale)
            # But rather think it should be:
            if (cpt != ppt) :
                py = math.ceil(py + min + (1 + math.log10(cpt - ppt) * scale))
            trace_df.at[index, 'y1'] = py

        ppt = row['physical_time']
        py = trace_df.at[index, 'y1']
        first_pass = False

    ############################################################################
    #### Derive arrows that match sided communications
    ############################################################################
    # Intialize all rows as pending to be matched
    trace_df['arrow'] = 'pending'
    trace_df['x2'] = -1
    trace_df['y2'] = -1

    # Iterate and check possible sides
    for index in trace_df.index:
        # If the tracepoint is pending, proceed to look for a match
        if (trace_df.at[index,'arrow'] == 'pending') :
            # Look for a match only if it is not about advancing time
            if (trace_df.at[index,'event'] == 'AdvLT') :
                trace_df.at[index,'arrow'] = 'adv'
                continue
            self_id = trace_df.at[index,'self_id']
            partner_id = trace_df.at[index,'partner_id']
            event =  trace_df.at[index,'event']
            logical_time = trace_df.at[index, 'logical_time']
            microstep = trace_df.at[index, 'microstep']
            inout = trace_df.at[index, 'inout']

            # Match tracepoints
            # Depends on whether the event is tagged or not
            if (trace_df.at[index,'event'] not in non_tagged_messages):
                matching_df = trace_df[\
                    (trace_df['inout'] != inout) & \
                    (trace_df['self_id'] == partner_id) & \
                    (trace_df['partner_id'] == self_id) & \
                    (trace_df['arrow'] == 'pending') & \
                    (trace_df['event'] == event) & \
                    (trace_df['logical_time'] == logical_time) & \
                    (trace_df['microstep'] == microstep) \
                ]
            elif (event == 'P2P_MSG'):
                # P2P messages travel directly between federates without going through the
                # RTI, so partner_id in the trace is typically -1 on both sides (the RTI is
                # not involved and the tracepoint has no partner). We therefore cannot use
                # partner_id for matching. Instead we match each 'out' to the first pending
                # 'in' whose physical_time >= the sender's physical_time (causality guarantee:
                # the receive cannot precede the send).
                physical_time = trace_df.at[index, 'physical_time']
                if (inout == 'out'):
                    matching_df = trace_df[\
                        (trace_df['inout'] == 'in') & \
                        (trace_df['arrow'] == 'pending') & \
                        (trace_df['event'] == event) & \
                        (trace_df['physical_time'] >= physical_time) \
                    ]
                else:
                    # 'in' rows are claimed by the corresponding 'out' pass above.
                    # If we reach an 'in' here it means no 'out' claimed it; render as dot.
                    matching_df = trace_df[0:0]
            else :
                matching_df = trace_df[\
                    (trace_df['inout'] != inout) & \
                    (trace_df['self_id'] == partner_id) & \
                    (trace_df['partner_id'] == self_id) & \
                    (trace_df['arrow'] == 'pending') & \
                    (trace_df['event'] == event)
                ]

            if (matching_df.empty) :
                # If no matching receiver, than set the arrow to 'dot',
                # meaning that only a dot will be rendered
                trace_df.at[index, 'arrow'] = 'dot'
            else:
                # If there is one or more matching rows, then consider 
                # the first one
                matching_index = matching_df.index[0]
                matching_row = matching_df.loc[matching_index]
                if (inout == 'out'):
                    trace_df.at[index, 'x2'] = matching_row['x1']
                    trace_df.at[index, 'y2'] = matching_row['y1']
                else:
                    trace_df.at[index, 'x2'] = trace_df.at[index, 'x1'] 
                    trace_df.at[index, 'y2'] = trace_df.at[index, 'y1'] 
                    trace_df.at[index, 'x1'] = matching_row['x1']
                    trace_df.at[index, 'y1'] = matching_row['y1']

                # Mark it, so not to consider it anymore
                trace_df.at[matching_index, 'arrow'] = 'marked'
                trace_df.at[index, 'arrow'] = 'arrow'

    ############################################################################
    #### Write output file
    ############################################################################
    # svg_width is the natural pixel width of the diagram content.
    # The HTML body uses overflow-x: auto so a horizontal scrollbar appears
    # when the browser window is narrower than the diagram.
    svg_width = extra_left + padding * 2 + (len(actors) - 1) * spacing + padding * 2 + 200
    svg_height = padding + trace_df.iloc[-1]['y1']
    # The sticky header is tall enough to contain the circles (centred at padding/2, r=20).
    header_height = padding

    if args.svg:
        write_svg_file(svg_width, svg_height, header_height, padding, x_coor, actors_names, trace_df,
                       show_physical_times=not args.no_physical_times)
        output_name = 'trace_svg.svg'
    else:
        write_html_file(svg_width, svg_height, header_height, padding, x_coor, actors_names, trace_df,
                        show_physical_times=not args.no_physical_times)
        output_name = 'trace_svg.html'

    # Write to a csv file, just to double check
    trace_df.to_csv('all.csv', index=True)
    print('Fedsd: Successfully generated the sequence diagram in ' + output_name + '.')
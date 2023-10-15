#!/usr/bin/python3

# Utility that reports the interactions (exchanged messages) between federates and the RTI in a 
# sequence-diagram-like format, or between enclaves in an enclaved execution.
# 
# The utility operates on lft trace files and outputs an html file embedding an svg image.

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
    .ABS {stroke: #d9dd1f; fill: #d9dd1f; } \
    .LTC { stroke: #073b4c; fill: #073b4c;} \
    .T_MSG { stroke: #ef476f; fill: #ef476f} \
    .NET { stroke: #118ab2; fill: #118ab2} \
    .PTAG { stroke: #06d6a0; fill: #06d6a0} \
    .TAG { stroke: #08a578; fill: #08a578} \
    .TIMESTAMP { stroke: grey; fill: grey } \
    .FED_ID {stroke: #80DD99; fill: #80DD99 } \
    .ADV {stroke-linecap="round" ; stroke: "red" ; fill: "red"} \
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
    "Sending TIMESTAMP": "TIMESTAMP",
    "Sending NET": "NET",
    "Sending LTC": "LTC",
    "Sending STOP_REQ": "STOP_REQ",
    "Sending STOP_REQ_REP": "STOP_REQ_REP",
    "Sending STOP_GRN": "STOP_GRN",
    "Sending FED_ID": "FED_ID",
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
    "Receiving ACK": "ACK",
    "Receiving TIMESTAMP": "TIMESTAMP",
    "Receiving NET": "NET",
    "Receiving LTC": "LTC",
    "Receiving STOP_REQ": "STOP_REQ",
    "Receiving STOP_REQ_REP": "STOP_REQ_REP",
    "Receiving STOP_GRN": "STOP_GRN",
    "Receiving FED_ID": "FED_ID",
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
    "Receiving UNIDENTIFIED": "UNIDENTIFIED",
    "Scheduler advancing time ends": "AdvLT"
}

prune_event_name.setdefault(" ", "UNIDENTIFIED")

import argparse         # For arguments parsing
import pandas as pd     # For csv manipulation
import os
import sys
from pathlib import Path
import math
import subprocess

# Define the arguments to pass in the command line
parser = argparse.ArgumentParser(description='Set of the lft trace files to render.')
parser.add_argument('-r','--rti', type=str, 
                    help='RTI\'s lft trace file.')
parser.add_argument('-f','--federates', nargs='+',
                    help='List of the federates\' lft trace files.')

# Events matching at the sender and receiver ends depend on whether they are tagged
# (the elapsed logical time and microstep have to be the same) or not. 
# Set of tagged events (messages)
non_tagged_messages = {'FED_ID', 'ACK', 'REJECT', 'ADR_RQ', 'ADR_AD', 'MSG', 'P2P_MSG'}


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
     * String: the svg string of the line©
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
        str_line = '\t<path d="M'+str(x2)+' '+str(y2)+' L'+str(x2+10)+' '+str(y2+5)+' L'+str(x2+10)+' '+str(y2-5)+' Z"' \
             + ' transform="rotate('+str(rotation)+')" transform-origin="'+str(x2)+' '+str(y2)+'"' \
             + style \
             + '/>\n'
    else :
        str_line = '\t<path d="M'+str(x2)+' '+str(y2)+' L'+str(x2-10)+' '+str(y2+5)+' L'+str(x2-10)+' '+str(y2-5)+' Z"' \
             + ' transform="rotate('+str( 180 + rotation)+')" transform-origin="'+str(x2)+' '+str(y2)+'"' \
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
    #print('rot = '+str(rotation)+' x1='+str(x1)+' y1='+str(y1)+' x2='+str(x2)+' y2='+str(y2))
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

def svg_string_draw_adv(x, y, label) :
    '''
    Constructs the svg html string to draw at a dash, meaning that logical time is advancing there.

    Args:
     * x: Int X coordinate of the dash
     * y: Int Y coordinate of the dash
     * label: String to draw 
    Returns:
     * String: the svg string of the triangle
    '''
    str_line1 = svg_string_draw_line(x-5, y, x+5, y, "ADV")
    str_line2 = svg_string_draw_side_label(x, y, label)
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

def convert_lft_file_to_csv(lft_file):
    '''
    Call trace_to_csv command to convert the given binary lft trace file to csv format.

    Args:
     * lft_file: the lft trace file
    Return:
     * File: the converted csv file, if the conversion succeeds, and empty string otherwise.
     * String: the error message, in case the conversion did not succeed, and empty string otherwise.
    '''
    convert_process = subprocess.run(['trace_to_csv', lft_file], stdout=subprocess.DEVNULL)

    if (convert_process.returncode == 0):
        csv_file = os.path.splitext(lft_file)[0] + '.csv'
        return csv_file, ''
    else:
        return '', str(convert_process.stderr)

def get_and_convert_lft_files(rti_lft_file, federates_lft_files):
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
    if (rti_lft_file):
        rti_csv_file, error = convert_lft_file_to_csv(rti_lft_file)
        if (not rti_csv_file):
            print('Fedsf: Error converting the RTI\'s lft file: ' + error)
        else:
            print('Fedsd: Successfully converted trace file ' + rti_lft_file + ' to ' + rti_csv_file + '.')
    
    federates_csv_files = []
    for file in federates_lft_files:
        fed_csv_file, error = convert_lft_file_to_csv(file)
        if (not fed_csv_file):
            print('Fedsf: Error converting the federate lft file ' + file + ': ' + error)
        else: 
            print('Fedsd: Successfully converted trace file ' + file + ' to ' + fed_csv_file + '.')
            federates_csv_files.append(fed_csv_file)
        
    return rti_csv_file, federates_csv_files

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

    rti_csv_file, federates_csv_files = get_and_convert_lft_files(args.rti, args.federates)
    
    # The RTI and each of the federates have a fixed x coordinate. They will be
    # saved in a dict
    x_coor = {}
    actors = []
    actors_names = {}
    padding = 50
    spacing = 200       # Spacing between federates

    # Set the RTI x coordinate
    x_coor[-1] = padding * 2
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
                # Add to the list of sequence diagram actors and add the name
                actors.append(fed_id)
                actors_names[fed_id] = Path(fed_trace).stem
                # Derive the x coordinate of the actor
                x_coor[fed_id] = (padding * 2) + (spacing * (len(actors) - 1))
                fed_df['x1'] = x_coor[fed_id]
                trace_df = pd.concat([trace_df, fed_df])
                fed_df = fed_df[0:0]
    
        
    ############################################################################
    #### RTI trace processing, if any
    ############################################################################
    if (rti_csv_file):
        rti_df = load_and_process_csv_file(rti_csv_file)
        rti_df['x1'] = x_coor[-1]
    else:
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

    trace_df = pd.concat([trace_df, rti_df])

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
    #### Write to svg file
    ############################################################################
    svg_width = padding * 2 + (len(actors) - 1) * spacing + padding * 2 + 200
    svg_height = padding + trace_df.iloc[-1]['y1']

    with open('trace_svg.html', 'w', encoding='utf-8') as f:
        # Print header
        f.write('<!DOCTYPE html>\n')
        f.write('<html>\n')
        f.write('<body>\n\n')
        
        f.write('<svg width="'+str(svg_width)+'" height="'+str(svg_height)+'">\n')

        f.write(css_style)
        
        # Print the circles and the names
        for key in x_coor:
            title = actors_names[key]
            if (key == -1):
                f.write(svg_string_comment('RTI Actor and line'))
                center = 15
            else:
                f.write(svg_string_comment('Federate '+str(key)+': ' + title + ' Actor and line'))
                center = 5
            f.write(svg_string_draw_line(x_coor[key], math.ceil(padding/2), x_coor[key], svg_height, False))
            f.write('\t<circle cx="'+str(x_coor[key])+'" cy="'+str(math.ceil(padding/2))+'" r="20" stroke="black" stroke-width="2" fill="white"/>\n')
            f.write('\t<text x="'+str(x_coor[key]-center)+'" y="'+str(math.ceil(padding/2)+5)+'" fill="black">'+title+'</text>\n')

        # Now, we need to iterate over the traces to draw the lines
        f.write(svg_string_comment('Draw interactions'))
        for index, row in trace_df.iterrows():
            # For time labels, display them on the left for the RTI, right for everthing else.
            anchor = 'start'
            if (row['self_id'] < 0):
                anchor = 'end'

            # formatted physical time.
            # FIXME: Using microseconds is hardwired here.
            physical_time = f'{int(row["physical_time"]/1000):,}'

            if (row['event'] in {'FED_ID', 'ACK', 'REJECT', 'ADR_RQ', 'ADR_AD', 'MSG', 'P2P_MSG'}):
                label = row['event']
            else:
                label = row['event'] + '(' + f'{int(row["logical_time"]):,}' + ', ' + str(row['microstep']) + ')'
            
            if (row['arrow'] == 'arrow'): 
                f.write(svg_string_draw_arrow(row['x1'], row['y1'], row['x2'], row['y2'], label, row['event']))
                if (row['inout'] in 'in'):
                    f.write(svg_string_draw_side_label(row['x2'], row['y2'], physical_time, anchor))
                else:
                    f.write(svg_string_draw_side_label(row['x1'], row['y1'], physical_time, anchor))
            elif (row['arrow'] == 'dot'):
                if (row['inout'] == 'in'):
                    label = "(in) from " + str(row['partner_id']) + ' ' + label
                else :
                    label = "(out) to " + str(row['partner_id']) + ' ' + label
                
                if (anchor == 'end'):
                    f.write(svg_string_draw_side_label(row['x1'], row['y1'], physical_time, anchor))
                    f.write(svg_string_draw_dot(row['x1'], row['y1'], label))
                else:
                    f.write(svg_string_draw_dot_with_time(row['x1'], row['y1'], physical_time, label))

            elif (row['arrow'] == 'marked'):
                f.write(svg_string_draw_side_label(row['x1'], row['y1'], physical_time, anchor))

            elif (row['arrow'] == 'adv'):
                f.write(svg_string_draw_adv(row['x1'], row['y1'], label))

        f.write('\n</svg>\n\n')

        # Print footer
        f.write('</body>\n')
        f.write('</html>\n')

    # Write to a csv file, just to double check
    trace_df.to_csv('all.csv', index=True)
    print('Fedsd: Successfully generated the sequence diagram in trace_svg.html.')

function sfun_keyboard_input_v1_2(block)
% modified from sfun_keyboard_input_v1_01.m of Marc Compere by Emanuele Ruffaldi
% created : 17 June 2003
% modified: 20 June 2003
% created: 19 May 2009 => Level 2 and terminate
%

% Level-2 M file S-Function with keyboard
%   Copyright 1990-2004 The MathWorks, Inc.
%   $Revision: 1.1.6.1 $ 

  setup(block);
  
%endfunction

function setup(block)
  
  block.NumDialogPrms  = 1;
  block.NumInputPorts  = 0;
  block.NumOutputPorts = 3;

  block.OutputPort(1).Dimensions       = 1;
  block.OutputPort(1).SamplingMode = 'Sample';
  block.OutputPort(1).DatatypeID  = 0;

    block.OutputPort(2).Dimensions       = 1;
  block.OutputPort(2).SamplingMode = 'Sample';
  block.OutputPort(2).DatatypeID  = 0;

    block.OutputPort(3).Dimensions       = 1;
  block.OutputPort(3).SamplingMode = 'Sample';
  block.OutputPort(3).DatatypeID  = 0;

  block.SampleTimes = [-1,0];
%end
  
  %% Register methods
%  block.RegBlockMethod('CheckParameters', @CheckPrms);
  block.RegBlockMethod('PostPropagationSetup',    @DoPostPropSetup);
  block.RegBlockMethod('InitializeConditions',    @InitConditions);  
  block.RegBlockMethod('Outputs',                 @Output);  
  block.RegBlockMethod('Update',                  @Update);  
  block.RegBlockMethod('Terminate',                 @Terminate);    
%endfunction

function DoPostPropSetup(block)

  %% Setup Dwork
  block.NumDworks = 1;
  block.Dwork(1).Name = 'last'; 
  block.Dwork(1).Dimensions      = 1;
  block.Dwork(1).DatatypeID      = 0;
  block.Dwork(1).Complexity      = 'Real';
  block.Dwork(1).UsedAsDiscState = true;

%endfunction

function InitConditions(block)

  %% Initialize Dwork
  block.Dwork(1).Data = 0;
   
  
%endfunction

function Output(block)
    next = block.Dwork(1).Data ;
    dt = block.DialogPrm(1).Data; % seconds
    pre = now;
    x = pre;
    if dt > 0 % dt==0 means just count time
        dt = dt/(24*3600); % s -> days
        if next > 0 % not first
            dtn = next-pre;
            if dtn > 0 % we need to wait
                pause(dtn*(24*3600)); % days -> s as pause           
                x = now;
            end
            % now advance of dt increments (real dt)
            while next < x
                next = next + dt;
            end
        else
            % at start just now + dt
            next = pre + dt;
        end
        % store in Output
        block.Dwork(1).Data = next;
    end
    % emit
  block.OutputPort(1).Data = x;
  block.OutputPort(2).Data = pre;
  block.OutputPort(3).Data = next;
  
%endfunction

function Update(block)

%endfunction



% Callback for turning Modal OFF
function turn_modal_off(obj,eventdata,handle)



% Callback for turning Modal ON
function turn_modal_on(obj,eventdata,handle)

% % Callback for 'WindowButtonMoveFcn' in figure
% function myCallback_move(obj,eventdata)
% str=sprintf('\tWindowButtonMoveFcn callback executing');disp(str)
% end
% 
% % Callback for 'WindowButtonDownFcn' in figure
% function myCallback_clickdown(obj,eventdata)
% str=sprintf('\t\tWindowButtonDownFcn callback executing');disp(str)
% end



function Terminate(block)

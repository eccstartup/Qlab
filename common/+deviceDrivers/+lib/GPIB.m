% CLASS GPIB - Simple wrapper for MATLAB's instrument control toolbox
% gpib class

% Original authors: Colm Ryan and Blake Johnson

% Copyright 2012 Raytheon BBN Technologies
%
% Licensed under the Apache License, Version 2.0 (the "License");
% you may not use this file except in compliance with the License.
% You may obtain a copy of the License at
%
%     http://www.apache.org/licenses/LICENSE-2.0
%
% Unless required by applicable law or agreed to in writing, software
% distributed under the License is distributed on an "AS IS" BASIS,
% WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
% See the License for the specific language governing permissions and
% limitations under the License.
%
classdef GPIB < hgsetget
    properties
        interface = []
        buffer_size = 1048576 % 1 MB buffer
    end
  
    methods
        %%
        function connect(obj, address)
            if ischar(address)
                address = str2double(address);
            end
            
            % create a GPIB object
            if ~isempty(obj.interface)
                fclose(obj.interface);
                delete(obj.interface);
            end
                
            obj.interface = gpib('ni', 0, address);
            obj.interface.InputBufferSize = obj.buffer_size;
            obj.interface.OutputBufferSize = obj.buffer_size;
            fopen(obj.interface);
        end

        function disconnect(obj)
            flushoutput(obj.interface);
            fclose(obj.interface);
        end

        function delete(obj)
            obj.disconnect();
        end
        
        function write(obj, string)
            fprintf(obj.interface, string);
        end
        
        function val = query(obj, string)
            val = query(obj.interface, string);
        end
        
        function val = read(obj)
            val = fgetl(obj.interface);
        end
        
        % binary read/write functions
        function binblockwrite(obj, varargin)
            binblockwrite(obj.interface, varargin{:});
        end
        
        function val = binblockread(obj, varargin)
            val = binblockread(obj.interface, varargin{:});
        end
    end
    
end

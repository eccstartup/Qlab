function [filename, segmentPoints] = Pi2CalChannelSequence(obj, qubit, direction, numPulses, makePlot)

if ~exist('direction', 'var')
    direction = 'X';
elseif ~strcmp(direction, 'X') && ~strcmp(direction, 'Y')
    warning('Unknown direction, assuming X');
    direction = 'X';
end
if ~exist('makePlot', 'var')
    makePlot = false;
end

basename = 'Pi2Cal';

fixedPt = 6000;

pg = PatternGen(qubit, 'pi2Amp', obj.pulseParams.pi2Amp);

pulseLib = containers.Map();
pulses = {'QId', 'X90p', 'X90m', 'Y90p', 'Y90m'};
for p = pulses
    pname = cell2mat(p);
    pulseLib(pname) = pg.pulse(pname);
end

patseq{1} = {pulseLib('QId')};

sindex = 1;

% +X rotations
% (1, 3, 5, 7, 9, 11, 13, 15, 17) x X90p
for j = 1:numPulses
    for k = 1:(1+2*(j-1))
        patseq{sindex + j}{k} = pulseLib([direction '90p']);
    end
end
sindex = sindex + numPulses;

% -X rotations
% (1, 3, 5, 7, 9, 11, ...) x X90m
for j = 1:numPulses
    for k = 1:(1+2*(j-1))
        patseq{sindex + j}{k} = pulseLib([direction '90m']);
    end
end

numsteps = 1;
nbrRepeats = 2;
segmentPoints = 1:nbrRepeats*length(patseq);
calseq = [];

% prepare parameter structures for the pulse compiler
seqParams = struct(...
    'basename', basename, ...
    'suffix', '', ...
    'numSteps', numsteps, ...
    'nbrRepeats', nbrRepeats, ...
    'fixedPt', fixedPt);

if ~isempty(calseq), calseq = {calseq}; end

qubitMap = obj.channelMap.(qubit);
IQkey = qubitMap.IQkey;

patternDict = containers.Map();
patternDict(IQkey) = struct('pg', pg, 'patseq', {patseq}, 'calseq', calseq, 'channelMap', qubitMap);
measChannels = {obj.settings.measurement};
awgs = fieldnames(obj.AWGs)';

compileSequences(seqParams, patternDict, measChannels, awgs, makePlot);

filename = obj.getAWGFileNames(basename);

end


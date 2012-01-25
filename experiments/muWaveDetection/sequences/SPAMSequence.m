function SPAMSequence(makePlot)

%Y90-(X180-Y180-X180-Y180)^n to determine phase difference between quadrature channels

if ~exist('makePlot', 'var')
    makePlot = true;
end
basename = 'SPAM';
fixedPt = 6000;
cycleLength = 10000;
nbrRepeats = 1;

% load config parameters from file
params = jsonlab.loadjson(getpref('qlab', 'pulseParamsBundleFile'));
qParams = params.q3; % choose target qubit here
IQkey = 'TekAWG34';
% if using SSB, uncomment the following line
% params.(IQkey).T = eye(2);
pg = PatternGen('dPiAmp', qParams.piAmp, 'dPiOn2Amp', qParams.pi2Amp, 'dSigma', qParams.sigma, 'dPulseType', qParams.pulseType, 'dDelta', qParams.delta, 'correctionT', params.(IQkey).T, 'dBuffer', qParams.buffer, 'dPulseLength', qParams.pulseLength, 'cycleLength', cycleLength, 'passThru', params.(IQkey).passThru);
%2.81199967498
angleShift = 0*pi/180;
SPAMBlock = {pg.pulse('Xp'),pg.pulse('Up','angle',pi/2+angleShift),pg.pulse('Xp'),pg.pulse('Up','angle',pi/2+angleShift)};

patseq = {};

for SPAMct = 0:10
    patseq{end+1} = {pg.pulse('Y90p')};
    for ct = 0:SPAMct
        patseq{end} = [patseq{end}, SPAMBlock];
    end
    patseq{end} = [patseq{end}, {pg.pulse('X90m')}];
end

calseq = {};
calseq{end+1} = {pg.pulse('QId')};
calseq{end+1} = {pg.pulse('QId')};
calseq{end+1} = {pg.pulse('Xp')};
calseq{end+1} = {pg.pulse('Xp')};

compiler = ['compileSequence' IQkey];
compileArgs = {basename, pg, patseq, calseq, 1, nbrRepeats, fixedPt, cycleLength, makePlot};
if exist(compiler, 'file') == 2 % check that the pulse compiler is on the path
    feval(compiler, compileArgs{:});
end
end

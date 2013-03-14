function handle = labeledDropDown(parent, position, label, fields)
	height = position(4);
	labelPosition = position + [0 height-5 0 0];
	% make label
	label = uicontrol( ...
		'Parent', parent, ...
		'Style', 'text', ...
		'Units', 'pixels', ...
		'Position', labelPosition, ...
		'FontName', 'Helvetica', ...
		'FontSize', 10, ...
		'String', label);

	handle = uicontrol( ...
			'Parent', parent, ...
			'Tag', 'fastLoop', ...
			'Style', 'popupmenu', ...
			'Units', 'pixels', ...
			'Position', position, ...
			'BackgroundColor', [1 1 1], ...
			'FontName', 'Helvetica', ...
			'FontSize', 10, ...
			'String', fields);
end
/*
*	NOVUSCORE LOGIN SCREEN
*	Version 0.2: Fancified.
*	Updated 22/10/2020	
*/

void LogIn()
{
	// LOGIN
	Entity backgroundId;
	Entity usernameFieldId;
	Entity passwordFieldId;
	DataStorage::GetEntity("LOGIN-background", backgroundId);
	DataStorage::GetEntity("LOGIN-usernameField", usernameFieldId);
	DataStorage::GetEntity("LOGIN-passwordField", passwordFieldId);
	
	string username;
	string password;

	InputField@ usernameField = cast<InputField>(UI::GetElement(usernameFieldId));
	username = usernameField.GetText();

	InputField@ passwordField = cast<InputField>(UI::GetElement(passwordFieldId));
	password = passwordField.GetText();
		
	Panel@ background = cast<Panel>(UI::GetElement(backgroundId));
	background.SetVisible(false);
	
	// TODO Log in.
}

void OnFieldSubmit(InputField@ inputField)
{
	LogIn();
}

void OnLoginButtonClick(Button@ button)
{
	LogIn();
}

void OnLoginScreenLoaded(uint SceneLoaded)
{
	vec2 SIZE = vec2(300,50);
	uint LABELFONTSIZE = 50;
	uint INPUTFIELDFONTSIZE = 35;
	string FONT = "Data/fonts/Ubuntu/Ubuntu-Regular.ttf";
	Color TEXTCOLOR = Color(1,0.78,0);
	Color OUTLINECOLOR = Color(0,0,0);
	float outlineWidth = 1.0f;

	Panel@ background = CreatePanel();
	Panel@ userNameFieldPanel = CreatePanel();
	Panel@ passwordFieldPanel = CreatePanel();
	Checkbox@ checkBox = CreateCheckbox();
	Label@ userNameLabel = CreateLabel();
	Label@ passwordLabel = CreateLabel();
	Label@ rememberAccountLabel = CreateLabel();
	Button@ submitButton = CreateButton();
	InputField@ usernameField = CreateInputField();
	InputField@ passwordField = CreateInputField();

	background.SetSize(UI::GetResolution()); // TODO: We need some fill parent for whole screen.
	background.SetTexture("Data/extracted/textures/Interface/Glues/LoadingScreens/LoadScreenOutlandWide.dds");
	background.SetDepthLayer(0);
	DataStorage::EmplaceEntity("LOGIN-background", background.GetEntityId());

	userNameLabel.SetParent(background);
	userNameLabel.SetAnchor(vec2(0.5,0.5));
	userNameLabel.SetTransform(vec2(0, -50), SIZE);
	userNameLabel.SetLocalAnchor(vec2(0.5,1));
	userNameLabel.SetFont(FONT, LABELFONTSIZE);
	userNameLabel.SetColor(TEXTCOLOR);
	userNameLabel.SetText("Username");
	userNameLabel.SetOutlineWidth(outlineWidth);
	userNameLabel.SetOutlineColor(OUTLINECOLOR);
	userNameLabel.SetHorizontalAlignment(1);
	
	userNameFieldPanel.SetParent(background);
	userNameFieldPanel.SetAnchor(vec2(0.5,0.5));
	userNameFieldPanel.SetTransform(vec2(0, -50), SIZE);
	userNameFieldPanel.SetLocalAnchor(vec2(0.5,0));
	userNameFieldPanel.SetTexture("Data/extracted/textures/Interface/Tooltips/chatbubble-background.dds");
	userNameFieldPanel.SetBorder("Data/extracted/textures/Interface/Glues/Common/Glue-Tooltip-Border.dds");
	userNameFieldPanel.SetBorderSize(16, 16, 16, 16);
	userNameFieldPanel.SetBorderInset(4, 5, 9, 10);
			
	usernameField.SetParent(userNameFieldPanel);
	usernameField.SetPosition(vec2(0,0));
	usernameField.SetFillParentSize(true);
	usernameField.SetFont(FONT, INPUTFIELDFONTSIZE);
	usernameField.OnSubmit(OnFieldSubmit);
	DataStorage::EmplaceEntity("LOGIN-usernameField", usernameField.GetEntityId());
			
	passwordLabel.SetParent(background);
	passwordLabel.SetAnchor(vec2(0.5,0.5));
	passwordLabel.SetTransform(vec2(0, 50), SIZE);
	passwordLabel.SetLocalAnchor(vec2(0.5,1));
	passwordLabel.SetFont(FONT, LABELFONTSIZE);
	passwordLabel.SetColor(TEXTCOLOR);
	passwordLabel.SetText("Password");
	passwordLabel.SetOutlineWidth(outlineWidth);
	passwordLabel.SetOutlineColor(OUTLINECOLOR);
	passwordLabel.SetHorizontalAlignment(1);
			
	passwordFieldPanel.SetParent(background);
	passwordFieldPanel.SetAnchor(vec2(0.5,0.5));
	passwordFieldPanel.SetTransform(vec2(0, 50), SIZE);
	passwordFieldPanel.SetLocalAnchor(vec2(0.5,0));
	passwordFieldPanel.SetTexture("Data/extracted/textures/Interface/Tooltips/chatbubble-background.dds");
	passwordFieldPanel.SetBorder("Data/extracted/textures/Interface/Glues/Common/Glue-Tooltip-Border.dds");
	passwordFieldPanel.SetBorderSize(16, 16, 16, 16);
	passwordFieldPanel.SetBorderInset(4, 5, 9, 10);
		
	passwordField.SetParent(passwordFieldPanel);
	passwordField.SetPosition(vec2(0,0));
	passwordField.SetFillParentSize(true);
	passwordField.SetFont(FONT, INPUTFIELDFONTSIZE);
	passwordField.OnSubmit(OnFieldSubmit);
	DataStorage::EmplaceEntity("LOGIN-passwordField", passwordField.GetEntityId());
	
	submitButton.SetParent(background);
	submitButton.SetAnchor(vec2(0.5,0.5));
	submitButton.SetTransform(vec2(0, SIZE.y * 2.5f), SIZE);
	submitButton.SetLocalAnchor(vec2(0.5,0));
	submitButton.SetTexture("Data/extracted/Textures/interface/glues/common/glue-panel-button-up-blue.dds");
	// TOP, RIGHT, BOTTOM, LEFT
	submitButton.SetBorderInset(0, 108, 16, 0);
	submitButton.SetFont(FONT, INPUTFIELDFONTSIZE);
	submitButton.SetText("Submit");
	submitButton.SetOutlineWidth(outlineWidth);
	submitButton.SetOutlineColor(OUTLINECOLOR);
	submitButton.OnClick(OnLoginButtonClick);
			
	checkBox.SetParent(background);
	checkBox.SetAnchor(vec2(0.5,0.5));
	checkBox.SetTransform(vec2(-SIZE.x/2, SIZE.y * 4), vec2(40,40));
	checkBox.SetTexture("Data/extracted/Textures/interface/buttons/ui-checkbox-up.dds");
	checkBox.SetCheckTexture("Data/extracted/Textures/interface/buttons/ui-checkbox-check.dds");
	checkBox.SetCheckColor(Color(0,1,0));
	checkBox.SetExpandBoundsToChildren(true);
	
	rememberAccountLabel.SetParent(checkBox);
	rememberAccountLabel.SetTransform(vec2(40,0), vec2(SIZE.x - 20, 40));
	rememberAccountLabel.SetFont(FONT, 25);
	rememberAccountLabel.SetColor(TEXTCOLOR);
	rememberAccountLabel.SetText("Remember Account Name");
	rememberAccountLabel.SetVerticalAlignment(1);
	
	background.MarkDirty();
	background.MarkBoundsDirty();		
}

void main()
{
	SceneManager::RegisterSceneLoadedCallback("LoginScreen", "LoginScreen-UI-Callback", OnLoginScreenLoaded);
}

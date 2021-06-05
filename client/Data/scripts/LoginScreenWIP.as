/*
*	NOVUSCORE LOGIN SCREEN
*	Version 0.4.1: Show that dialog someone wanted.
*	Updated 07/05/2021	
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
	
	// Show DIALOG.
	Entity dialogId;
	DataStorage::GetEntity("DIALOG", dialogId);
    
	Panel@ dialogPanel = cast<Panel>(UI::GetElement(dialogId));
	dialogPanel.SetVisible(true);
}

void OnFieldSubmit(InputField@ inputField)
{
	LogIn();
}

void OnLoginButtonClick(EventElement@ button)
{
	LogIn();
}

void OnLoginScreenLoaded(uint SceneLoaded)
{
	vec2 SIZE = vec2(300,55);
	const uint LABELFONTSIZE = 35;
	const uint INPUTFIELDFONTSIZE = 35;
	const string FONT = "Data/fonts/Ubuntu/Ubuntu-Regular.ttf";
	const Color TEXTCOLOR = Color(1,0.78,0);
	const Color OUTLINECOLOR = Color(0,0,0);
	const float outlineWidth = 1.0f;

	ImageStylesheet fieldSheet;
	fieldSheet.SetTexture("Data/extracted/textures/Interface/Tooltips/chatbubble-background.dds");
	fieldSheet.SetBorderTexture("Data/extracted/textures/Interface/Glues/Common/Glue-Tooltip-Border.dds");
	fieldSheet.SetBorderSize(Box(16,16,16,16));
	fieldSheet.SetBorderInset(Box(4,5,9,10));

	ImageStylesheet backgroundSheet("Data/extracted/textures/Interface/Glues/LoadingScreens/loadscreennorthrendwide.dds");
	
	ImageStylesheet checkBackSheet("Data/extracted/Textures/interface/buttons/ui-checkbox-up.dds");
	ImageStylesheet checkCheckSheet("Data/extracted/Textures/interface/buttons/ui-checkbox-check.dds");
	checkCheckSheet.SetColor(Color(0,1,0));

	TextStylesheet labelSheet(FONT, LABELFONTSIZE);
	labelSheet.SetColor(TEXTCOLOR);
	labelSheet.SetOutlineWidth(outlineWidth);
	labelSheet.SetOutlineColor(OUTLINECOLOR);
	labelSheet.SetHorizontalAlignment(1);

	ImageStylesheet buttonSheet("Data/extracted/Textures/interface/glues/common/glue-panel-button-up-blue.dds");
	buttonSheet.SetTexCoord(FBox(0.0f, 0.578125f, 0.75f, 0.0f));
	ImageStylesheet buttonDownSheet("Data/extracted/Textures/interface/glues/common/glue-panel-button-down-blue.dds");
	buttonDownSheet.SetTexCoord(FBox(0.0f, 0.578125f, 0.75f, 0.0f));

	TextStylesheet buttonTextSheet(FONT, INPUTFIELDFONTSIZE * 0.9f);
	buttonTextSheet.SetColor(TEXTCOLOR);
	buttonTextSheet.SetOutlineColor(OUTLINECOLOR);
	buttonTextSheet.SetOutlineWidth(outlineWidth);
	buttonTextSheet.SetHorizontalAlignment(1);

	TextStylesheet inputFieldSheet(FONT, INPUTFIELDFONTSIZE);

	Panel@ background = CreatePanel("LoadScreenBackground");
	Panel@ userNameFieldPanel = CreatePanel("UsernamePanel",false);
	Panel@ passwordFieldPanel = CreatePanel("PasswordPanel",false);
	Label@ userNameLabel = CreateLabel("UsernameLabel");
	Label@ passwordLabel = CreateLabel("PasswordLabel");
	Label@ rememberAccountLabel = CreateLabel("RememberAccountNameLabel");
	Checkbox@ checkBox = CreateCheckbox("RememberAccountNameCheckbox");
	Button@ submitButton = CreateButton("LoginButton");
	InputField@ usernameField = CreateInputField("UsernameField");
	InputField@ passwordField = CreateInputField("PasswordField");

	background.SetSize(UI::GetResolution());
	background.SetAnchor(vec2(0.5f,0.5f));
	background.SetLocalAnchor(vec2(0.5f, 0.5f));
	background.SetStylesheet(backgroundSheet);
	background.SetDepthLayer(0);
	DataStorage::EmplaceEntity("LOGIN-background", background.GetEntityId());

	userNameLabel.SetTransform(vec2(0, -35), SIZE);
	userNameLabel.SetAnchor(vec2(0.5,0.5));
	userNameLabel.SetLocalAnchor(vec2(0.5,1));
	userNameLabel.SetText("Account Name");
	userNameLabel.SetStylesheet(labelSheet);
	background.AddChild(userNameLabel);
	
	userNameFieldPanel.SetTransform(vec2(0, -50), SIZE);
	userNameFieldPanel.SetAnchor(vec2(0.5,0.5));
	userNameFieldPanel.SetLocalAnchor(vec2(0.5,0));
	userNameFieldPanel.SetStylesheet(fieldSheet);
	userNameFieldPanel.SetPadding(FBox(2, 5, 9, 10));
	background.AddChild(userNameFieldPanel);
			
	usernameField.SetFillParentSize(true);
	usernameField.SetStylesheet(inputFieldSheet);
	usernameField.OnSubmit(OnFieldSubmit);
	DataStorage::EmplaceEntity("LOGIN-usernameField", usernameField.GetEntityId());
	userNameFieldPanel.AddChild(usernameField);
			
	passwordLabel.SetTransform(vec2(0, 65), SIZE);
	passwordLabel.SetAnchor(vec2(0.5,0.5));
	passwordLabel.SetLocalAnchor(vec2(0.5,1));
	passwordLabel.SetStylesheet(labelSheet);
	passwordLabel.SetText("Account Password");
	background.AddChild(passwordLabel);
			
	passwordFieldPanel.SetTransform(vec2(0, 50), SIZE);
	passwordFieldPanel.SetAnchor(vec2(0.5,0.5));
	passwordFieldPanel.SetLocalAnchor(vec2(0.5,0));
	passwordFieldPanel.SetStylesheet(fieldSheet);
	passwordFieldPanel.SetPadding(FBox(2, 5, 9, 10));
	background.AddChild(passwordFieldPanel);
		
	passwordField.SetFillParentSize(true);
	passwordField.SetStylesheet(inputFieldSheet);
	passwordField.OnSubmit(OnFieldSubmit);
	DataStorage::EmplaceEntity("LOGIN-passwordField", passwordField.GetEntityId());
	passwordFieldPanel.AddChild(passwordField);
	
	submitButton.SetTransform(vec2(0, SIZE.y * 2.25f), SIZE * vec2(1.1f,1.4f));
	submitButton.SetAnchor(vec2(0.5,0.5));
	submitButton.SetLocalAnchor(vec2(0.5,0));
	submitButton.SetPadding(FBox(8, 0, 0, 0));
	submitButton.SetText("Login");
	submitButton.SetStylesheet(buttonSheet);
	submitButton.SetPressedStylesheet(buttonDownSheet);
	submitButton.SetTextStylesheet(buttonTextSheet);
	submitButton.OnClick(OnLoginButtonClick);
	submitButton.SetFocusable(false);
	background.AddChild(submitButton);
			
	checkBox.SetTransform(vec2(-SIZE.x/2 + 20, SIZE.y * 3.4f), vec2(30,30));
	checkBox.SetAnchor(vec2(0.5,0.5));
	checkBox.SetStylesheet(checkBackSheet);
	checkBox.SetCheckStylesheet(checkCheckSheet);
	checkBox.SetCollisionIncludesChildren(true);
	background.AddChild(checkBox);
	
	rememberAccountLabel.SetAnchor(vec2(1,0));
	rememberAccountLabel.SetTransform(vec2(5,0), vec2(SIZE.x - 80, 30));
	labelSheet.SetFontSize(20.0f);
	rememberAccountLabel.SetStylesheet(labelSheet);
	rememberAccountLabel.SetText("Remember Account Name");
	checkBox.AddChild(rememberAccountLabel);
	
	background.MarkDirty();
	background.MarkBoundsDirty();		
}

void main()
{
	SceneManager::RegisterSceneLoadedCallback("LoginScreen", "LoginScreen-UI-Callback", OnLoginScreenLoaded);
}

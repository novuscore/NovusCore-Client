/*
*	NOVUSCORE LOGIN SCREEN
*	Version 0.1
*	Updated 22/07/2020
*/

void Login()
{
	//LOGIN
	Entity usernameFieldId;
	DataStorage::GetEntity("LOGIN-usernameField", usernameFieldId);

	Print("Login!");
}

void OnFieldSubmit(InputField@ inputField)
{
	Login();
}

void OnLoginButtonClick(Button@ button)
{
	Print("Button Clicked!");
	Login();
}

void OnLoginScreenLoaded(uint SceneLoaded)
{
	uint CENTERX = 960;
	uint CENTERY = 540;

	vec2 SIZE = vec2(300,50);

	Color TEXTCOLOR = Color(0,0,0.5);
	string FONT = "Data/fonts/Ubuntu/Ubuntu-Regular.ttf";

	//Background
	Panel@ ICCLoadScreen = CreatePanel();
	ICCLoadScreen.SetSize(vec2(1920,1080));
	ICCLoadScreen.SetTexture("Data/extracted/textures/Interface/Glues/LoadingScreens/LoadScreenIcecrownCitadel.dds");

	//Username
	Label@ userNameLabel = CreateLabel();
	userNameLabel.SetTransform(vec2(CENTERX, CENTERY - 50), SIZE);
	userNameLabel.SetLocalAnchor(vec2(0.5,1));
	userNameLabel.SetFont(FONT, 50);
	userNameLabel.SetColor(TEXTCOLOR);
	userNameLabel.SetText("Username");

	Panel@ userNameFieldPanel = CreatePanel();
	userNameFieldPanel.SetTransform(vec2(CENTERX, CENTERY - 50), SIZE);
	userNameFieldPanel.SetLocalAnchor(vec2(0.5,0));
	//userNameFieldPanel.SetTexture("Data/extracted/textures/Interface/Tooltips/UI-Tooltip-Background.dds");
	userNameFieldPanel.SetTexture("Data/textures/NovusUIPanel.png");

	InputField@ usernameField = CreateInputField();
	usernameField.SetParent(userNameFieldPanel);
	usernameField.SetPosition(vec2(0,0));
	usernameField.SetFillParentSize(true);
	usernameField.SetFont(FONT, 50);
	usernameField.OnSubmit(OnFieldSubmit);
	DataStorage::EmplaceEntity("LOGIN-usernameField", usernameField.GetEntityId());
	
	//Password
	Label@ passwordLabel = CreateLabel();
	passwordLabel.SetTransform(vec2(CENTERX, CENTERY + 50), SIZE);
	passwordLabel.SetLocalAnchor(vec2(0.5,1));
	passwordLabel.SetFont(FONT, 50);
	passwordLabel.SetColor(TEXTCOLOR);
	passwordLabel.SetText("Password");

	Panel@ passwordFieldPanel = CreatePanel();
	passwordFieldPanel.SetTransform(vec2(CENTERX, CENTERY + 50), SIZE);
	passwordFieldPanel.SetLocalAnchor(vec2(0.5,0));
	//userNameFieldPanel.SetTexture("Data/extracted/textures/Interface/Tooltips/UI-Tooltip-Background.dds");
	passwordFieldPanel.SetTexture("Data/textures/NovusUIPanel.png");

	InputField@ passwordField = CreateInputField();
	passwordField.SetParent(passwordFieldPanel);
	passwordField.SetPosition(vec2(0,0));
	passwordField.SetFillParentSize(true);
	passwordField.SetFont(FONT, 50);
	passwordField.OnSubmit(OnFieldSubmit);
	DataStorage::EmplaceEntity("LOGIN-passwordField", passwordField.GetEntityId());

	//Submit
	Button@ submitButton = CreateButton();
	submitButton.SetPosition(vec2(CENTERX, CENTERY +  SIZE.y * 2.5f));
	submitButton.SetSize(SIZE);
	submitButton.SetLocalAnchor(vec2(0.5,0));
	submitButton.SetTexture("Data/textures/NovusUIPanel.png");
	submitButton.SetFont(FONT, 50);
	submitButton.SetText("Submit");
	submitButton.OnClick(OnLoginButtonClick);

	//Remember account name.
	Checkbox@ checkBox = CreateCheckbox();
	checkBox.SetPosition(vec2(CENTERX - SIZE.x/2 + 5, CENTERY +  SIZE.y * 4));
	checkBox.SetSize(vec2(25,25));
	checkBox.SetBackgroundTexture("Data/textures/NovusUIPanel.png");
	checkBox.SetCheckTexture("Data/textures/NovusUIPanel.png");
	checkBox.SetCheckColor(Color(0,1,0));
	checkBox.SetExpandBoundsToChildren(true);

	Label@ rememberAccountLabel = CreateLabel();
	rememberAccountLabel.SetParent(checkBox);
	rememberAccountLabel.SetTransform(vec2(25,0), vec2(SIZE.x - 25, 25));
	rememberAccountLabel.SetFont(FONT, 25);
	rememberAccountLabel.SetColor(TEXTCOLOR);
	rememberAccountLabel.SetText("Remember Account Name");
}

void main()
{
	SceneManager::RegisterSceneLoadedCallback("LoginScreen", "LoginScreen-UI-Callback", OnLoginScreenLoaded);
}

void Login()
{
	//LOGIN
	Entity usernameFieldId;
	DataStorage::GetEntity("LGSC-usernameField", usernameFieldId);
}

void OnPasswordFieldSubmit(InputField@ inputField)
{
	Login();
}

void OnLoginButtonClick(Button@ button)
{
	Login();
}

void OnLoginScreenLoaded(uint SceneLoaded)
{
	uint CENTERX = 1920/2;
	uint CENTERY = 1080/2;

	Color TEXTCOLOR = Color(0,0,0.5);
	
	string FONTPATH = "Data/fonts/Ubuntu/Ubuntu-Regular.ttf";

	Panel@ ICCLoadScreen = CreatePanel();
	ICCLoadScreen.SetSize(vec2(1920,1080));
	ICCLoadScreen.SetTexture("Data/extracted/textures/Interface/Glues/LoadingScreens/LoadScreenIcecrownCitadel.dds");

	//Username
	Label@ userNameLabel = CreateLabel();
	userNameLabel.SetPosition(vec2(CENTERX, CENTERY - 50));
	userNameLabel.SetSize(vec2(300, 50));
	userNameLabel.SetLocalAnchor(vec2(0.5,1));
	userNameLabel.SetFont(FONTPATH, 50);
	userNameLabel.SetColor(TEXTCOLOR);
	userNameLabel.SetText("Username");

	Panel@ userNameFieldPanel = CreatePanel();
	userNameFieldPanel.SetPosition(vec2(CENTERX, CENTERY - 50));
	userNameFieldPanel.SetSize(vec2(300, 50));
	userNameFieldPanel.SetLocalAnchor(vec2(0.5,0));
	//userNameFieldPanel.SetTexture("Data/extracted/textures/Interface/Tooltips/UI-Tooltip-Background.dds");
	userNameFieldPanel.SetTexture("Data/textures/NovusUIPanel.png");

	InputField@ usernameField = CreateInputField();
	usernameField.SetParent(userNameFieldPanel);
	usernameField.SetPosition(vec2(0,0));
	usernameField.SetSize(vec2(300,50)); //TODO Fill to parent size?
	usernameField.SetFont(FONTPATH, 50);
	DataStorage::EmplaceEntity("LGSC-usernameField", usernameField.GetEntityId());
	
	//Password
	Label@ passwordLabel = CreateLabel();
	passwordLabel.SetSize(vec2(300, 50));
	passwordLabel.SetPosition(vec2(CENTERX, CENTERY + 50));
	passwordLabel.SetLocalAnchor(vec2(0.5,1));
	passwordLabel.SetFont("Data/fonts/Ubuntu/Ubuntu-Regular.ttf", 50);
	passwordLabel.SetColor(TEXTCOLOR);
	passwordLabel.SetText("Password");

	Panel@ passwordFieldPanel = CreatePanel();
	passwordFieldPanel.SetPosition(vec2(CENTERX, CENTERY + 50));
	passwordFieldPanel.SetSize(vec2(300, 50));
	passwordFieldPanel.SetLocalAnchor(vec2(0.5,0));
	//userNameFieldPanel.SetTexture("Data/extracted/textures/Interface/Tooltips/UI-Tooltip-Background.dds");
	passwordFieldPanel.SetTexture("Data/textures/NovusUIPanel.png");

	InputField@ passwordField = CreateInputField();
	passwordField.SetParent(passwordFieldPanel);
	passwordField.SetPosition(vec2(0,0));
	passwordField.SetSize(vec2(300,50)); //TODO Fill to parent size?
	passwordField.SetFont(FONTPATH, 50);
	passwordField.OnSubmit(OnPasswordFieldSubmit);
	DataStorage::EmplaceEntity("LGSC-password", passwordField.GetEntityId());
}

void main()
{
	SceneManager::RegisterSceneLoadedCallback("LoginScreen", "LoginScreen-UI-Callback", OnLoginScreenLoaded);
}

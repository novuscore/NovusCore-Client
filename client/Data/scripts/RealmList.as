void CloseButtonClicked(Button@ button)
{
    button.GetParent().SetVisible(false);
}

void AddRealmButton(Panel@ parent, uint index, string name, uint type, uint count, uint load)
{
    // Testing.

    Panel@ realmButton = CreatePanel("RealmList-RealmButton" + index);
    realmButton.SetTransform(vec2(0, 20*index), vec2(512, 20));

	TextStylesheet labelSheet("Data/fonts/Ubuntu/Ubuntu-Regular.ttf", 20);
    labelSheet.SetColor(Color(1, 0.78, 0));

    {
        Label@ realmName = CreateLabel("RealmList-RealmName" + index);
        realmName.SetSize(vec2(512, 20));
        realmName.SetStylesheet(labelSheet);
        realmName.SetText(name);
        realmButton.AddChild(realmName);
    }

    labelSheet.SetHorizontalAlignment(1);
    {
        string typeString = "";
        if(type == 0)
            typeString = "PvE";
        else if(type == 1)
        {
            typeString = "PvP";
            labelSheet.SetColor(Color(1,0,0));
        }
        else
            typeString = "RPPvP";

        Label@ realmType = CreateLabel("RealmList-RealmType" + index);
        realmType.SetTransform(vec2(235,0), vec2(60, 20));
        realmType.SetStylesheet(labelSheet);
        realmType.SetText(typeString);
        realmButton.AddChild(realmType);
    }

    labelSheet.SetColor(Color(1,1,1));
    {
        Label@ realmCharacterCount = CreateLabel("RealmList-RealmCharacterCount" + index);
        realmCharacterCount.SetTransform(vec2(235+100,0), vec2(45, 20));
        realmCharacterCount.SetStylesheet(labelSheet);
        realmCharacterCount.SetText("(" + count + ")");
        realmButton.AddChild(realmCharacterCount);
    }

    {
        string loadString = "";
        if(load == 0)
        {
            loadString = "Low";
            labelSheet.SetColor(Color(0,1,0));
        }
        else if(load == 1)
        {
            loadString = "Medium";
            labelSheet.SetColor(Color(1,1,1));
        }
        else if(load == 2)
        {
            loadString = "High";
            labelSheet.SetColor(Color(1,0,0));
        }
        else if(load == 3)
        {
            loadString = "Full";
            labelSheet.SetColor(Color(1,0,0));
        }
        else
        {
            loadString = "???";
            labelSheet.SetColor(Color(1,1,0));
        }

        Label@ realmCharacterCount = CreateLabel("RealmList-RealmCharacterCount" + index);
        realmCharacterCount.SetTransform(vec2(235+100 + 45,0), vec2(110, 20));
        realmCharacterCount.SetStylesheet(labelSheet);
        realmCharacterCount.SetText(loadString);
        realmButton.AddChild(realmCharacterCount);
    }

    parent.AddChild(realmButton);
}

void main()
{
    Panel@ parent = CreatePanel("RealmList");
    parent.SetTransform(vec2(24,0), vec2(640, 512));
    parent.SetAnchor(vec2(0.5,0.5));
    parent.SetLocalAnchor(vec2(0.5,0.5));
    parent.SetPadding(FBox(0, 34, 0, 0));
    {
        // I hate ye blizzard.
        Panel@ topLeftBG = CreatePanel("RealmList-TopLeft", false);
        topLeftBG.SetFillParentSize(true);
        topLeftBG.SetFillBounds(FBox(0, 0.4, 0.5, 0));
        topLeftBG.SetStylesheet(ImageStylesheet("Data/extracted/Textures/interface/helpframe/helpframe-topleft.dds"));
        parent.AddChild(topLeftBG);
        DataStorage::EmplaceEntity("REALMLIST", parent.GetEntityId());

        Panel@ topBG = CreatePanel("RealmList-Top", false);
        topBG.SetFillParentSize(true);
        topBG.SetFillBounds(FBox(0, 0.8, 0.5, 0.4));
        topBG.SetStylesheet(ImageStylesheet("Data/extracted/Textures/interface/helpframe/helpframe-top.dds"));
        parent.AddChild(topBG);

        Panel@ topRightBG = CreatePanel("RealmList-TopRight", false);
        topRightBG.SetFillParentSize(true);
        topRightBG.SetFillBounds(FBox(0, 1, 0.5, 0.8));
        topRightBG.SetStylesheet(ImageStylesheet("Data/extracted/Textures/interface/helpframe/helpframe-topright.dds"));
        parent.AddChild(topRightBG);

        Panel@ botLeftBG = CreatePanel("RealmList-BotLeft", false);
        botLeftBG.SetFillParentSize(true);
        botLeftBG.SetFillBounds(FBox(0.5, 0.4, 1, 0));
        botLeftBG.SetStylesheet(ImageStylesheet("Data/extracted/Textures/interface/helpframe/helpframe-botleft.dds"));
        parent.AddChild(botLeftBG);

        Panel@ botBG = CreatePanel("RealmList-Bot", false);
        botBG.SetFillParentSize(true);
        botBG.SetFillBounds(FBox(0.5, 0.8, 1, 0.4));
        botBG.SetStylesheet(ImageStylesheet("Data/extracted/Textures/interface/helpframe/helpframe-bottom.dds"));
        parent.AddChild(botBG);

        Panel@ botRightBG = CreatePanel("RealmList-BotRight", false);
        botRightBG.SetFillParentSize(true);
        botRightBG.SetFillBounds(FBox(0.5, 1, 1, 0.8));
        botRightBG.SetStylesheet(ImageStylesheet("Data/extracted/Textures/interface/helpframe/helpframe-botright.dds"));
        parent.AddChild(botRightBG);
    }

    {
        Button@ closeButton = CreateButton("REALMLIST-ButtonClose");
        closeButton.SetTransform(vec2(-40, 3), vec2(32,32));
        closeButton.SetAnchor(vec2(1,0));
        closeButton.SetLocalAnchor(vec2(1,0));
        closeButton.OnClick(CloseButtonClicked);

        closeButton.SetStylesheet(ImageStylesheet("Data/extracted/Textures/interface/buttons/ui-panel-minimizebutton-up.dds"));
        closeButton.SetPressedStylesheet(ImageStylesheet("Data/extracted/Textures/interface/buttons/ui-panel-minimizebutton-down.dds"));

        parent.AddChild(closeButton);
    }

    // TODO: Replace with ScrollBox when we have that.
    {
        Panel@ realmList = CreatePanel("REALMLIST-PanelRealmList");
        realmList.SetTransform(vec2(19, 54), vec2(564-31, 464-55));

        AddRealmButton(realmList, 0, "NovusRealm", 1, 255, 3);
        AddRealmButton(realmList, 1, "ICC25H Testing", 2, 1, 2);
        AddRealmButton(realmList, 2, "ICC25M Testing", 2, 5, 1);
        AddRealmButton(realmList, 3, "ICC25M+ Testing", 0, 7, 0);

        parent.AddChild(realmList);
    }
    parent.MarkDirty();
    parent.MarkBoundsDirty();
}

-- =====================================================================
-- Branchat migration 001: image attachments on messages
--
-- Adds:
--   * Branchat.Attachments table (image bytes linked to a message)
--   * Branchat.SaveAttachment, Branchat.GetBaseAttachments,
--     Branchat.GetTreeAttachments
-- Changes:
--   * Branchat.SaveMessage now RETURNs the new message ID
--   * Branchat.GetBaseMessages / GetTreeMessages now also return ID
--     (and use it as a tie-breaker when two messages share a timestamp)
--
-- Run once against the Branchat database. Safe to re-run.
-- DeleteChatBranch and DeleteGroup need no changes: deleting a message
-- cascades to its attachments.
-- =====================================================================

IF OBJECT_ID(N'Branchat.Attachments', N'U') IS NULL
BEGIN
    CREATE TABLE Branchat.Attachments
    (
        ID         INT IDENTITY
            CONSTRAINT PK_Attachments PRIMARY KEY,
        Message_ID INT            NOT NULL
            CONSTRAINT FK_Attachments_Messages
                REFERENCES Branchat.Messages
                ON DELETE CASCADE,
        Mime_Type  VARCHAR(100)   NOT NULL,
        File_Name  VARCHAR(260)   NULL,
        Data       VARBINARY(MAX) NOT NULL
    );

    CREATE INDEX IX_Attachments_Message_ID ON Branchat.Attachments (Message_ID);
END
GO

-- Self-heal: an earlier version of this script made File_Name NVARCHAR, which
-- the app's nanodbc build reads back truncated. Convert it to VARCHAR.
IF EXISTS (
    SELECT 1 FROM sys.columns c
    JOIN sys.types t ON t.user_type_id = c.user_type_id
    WHERE c.object_id = OBJECT_ID(N'Branchat.Attachments')
      AND c.name = 'File_Name' AND t.name = 'nvarchar'
)
    ALTER TABLE Branchat.Attachments ALTER COLUMN File_Name VARCHAR(260) NULL;
go

CREATE OR ALTER PROCEDURE Branchat.SaveMessage
    @Root_ID INT,
    @Chat_ID INT,
    @Role NVARCHAR(50),
    @Text VARCHAR(MAX),
    @Timestamp DATETIME2
AS
BEGIN
    SET NOCOUNT ON;
    INSERT INTO Branchat.Messages (Root_ID, Chat_ID, Role, Text, Timestamp)
    VALUES (@Root_ID, @Chat_ID, @Role, @Text, @Timestamp);

    -- Return the new message ID so attachments can be linked to it
    RETURN CAST(SCOPE_IDENTITY() AS INT);
END;
go

CREATE OR ALTER PROCEDURE Branchat.SaveAttachment
    @Message_ID INT,
    @Mime_Type VARCHAR(100),
    @File_Name VARCHAR(260),
    @Data VARBINARY(MAX)
AS
BEGIN
    SET NOCOUNT ON;
    INSERT INTO Branchat.Attachments (Message_ID, Mime_Type, File_Name, Data)
    VALUES (@Message_ID, @Mime_Type, @File_Name, @Data);

    RETURN CAST(SCOPE_IDENTITY() AS INT);
END;
go

CREATE OR ALTER PROCEDURE Branchat.GetBaseMessages
AS
BEGIN
    SET NOCOUNT ON;

    -- Text stays last: it is a MAX column and must be read after the others
    SELECT
        ID,
        Root_ID,
        Chat_ID,
        Role,
        Timestamp,
        Text
    FROM
        Branchat.Messages
    WHERE
        Chat_ID = Root_ID
    ORDER BY
        Chat_ID ASC, Timestamp ASC, ID ASC;
END;
go

CREATE OR ALTER PROCEDURE Branchat.GetTreeMessages
@RootID INT
AS
BEGIN
    SET NOCOUNT ON;
    SELECT
        ID,
        Root_ID,
        Chat_ID,
        Role,
        Timestamp,
        Text
    FROM
        Branchat.Messages
    WHERE
        Root_ID = @RootID
    ORDER BY
        Chat_ID, Timestamp, ID;
END;
go

-- Attachments for root-chat messages (same set as GetBaseMessages)
CREATE OR ALTER PROCEDURE Branchat.GetBaseAttachments
AS
BEGIN
    SET NOCOUNT ON;

    -- Data stays last: it is a MAX column and must be read after the others
    SELECT
        a.Message_ID,
        a.ID,
        a.Mime_Type,
        a.File_Name,
        a.Data
    FROM
        Branchat.Attachments a
        INNER JOIN Branchat.Messages m ON m.ID = a.Message_ID
    WHERE
        m.Chat_ID = m.Root_ID
    ORDER BY
        a.Message_ID, a.ID;
END;
go

-- Attachments for every message in one chat tree (same set as GetTreeMessages)
CREATE OR ALTER PROCEDURE Branchat.GetTreeAttachments
@RootID INT
AS
BEGIN
    SET NOCOUNT ON;
    SELECT
        a.Message_ID,
        a.ID,
        a.Mime_Type,
        a.File_Name,
        a.Data
    FROM
        Branchat.Attachments a
        INNER JOIN Branchat.Messages m ON m.ID = a.Message_ID
    WHERE
        m.Root_ID = @RootID
    ORDER BY
        a.Message_ID, a.ID;
END;
go

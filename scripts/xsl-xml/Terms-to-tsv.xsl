<?xml version="1.0" encoding="UTF-8"?>

<xsl:stylesheet version="1.0"
xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

<xsl:output method="text" indent="yes" encoding="utf-8" standalone="yes"/>
<xsl:template match="/">

	<xsl:text>Identifier	Name	Proportion
</xsl:text>
      <xsl:for-each select="root/geneOntologyTerm">

        <xsl:sort data-type="number" order="descending" select="proportion" />

<xsl:if test="proportion/text() != '0'">

<xsl:value-of select="identifier"/>
	<xsl:text>	</xsl:text>
<xsl:value-of select="name"/>
	<xsl:text>	</xsl:text>
          	<xsl:value-of select="proportion"/>
	<xsl:text>
</xsl:text>

</xsl:if>

      </xsl:for-each>
</xsl:template>

</xsl:stylesheet> 

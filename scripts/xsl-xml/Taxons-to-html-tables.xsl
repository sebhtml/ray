<?xml version="1.0" encoding="UTF-8"?>


<xsl:stylesheet version="1.0"
xmlns:xsl="http://www.w3.org/1999/XSL/Transform">

<xsl:template match="/">
  <html>
<head>

<style>

body{
font-family: Arial;
}

table{
margin: 40px;
}

th{
background-color:green;
color:white;
border: 1px solid black;

padding:15px;
}

td{
width: 200px;
padding:15px;
border: 1px solid black;

}

</style>

<title>
Sample: <xsl:value-of select="root/sample"/>
</title>



</head>
  <body>
    <h1>Sample: <xsl:value-of select="root/sample"/></h1>
    <h2>Taxonomic profiling from BiologicalAbundances/_Taxonomy/Taxons.xml</h2>
	<h2>Produced by Ray technologies</h2>


<div>
Total assembled k-mer observations:
   <xsl:value-of select="root/totalAssembledKmerObservations"/>
</div>
<div>
Total colored assembled k-mer observations:
   <xsl:value-of select="root/totalColoredAssembledKmerObservations"/>
</div>





<table><caption>rank: kingdom</caption><tbody>
<tr><th>Taxon</th><th>Colored k-mer observations</th>
<th>Colored proportion</th></tr>

      <xsl:for-each select="root/entry">

        <xsl:sort data-type="number" order="descending" select="recursive/assembledKmerObservations" />
	<xsl:if test="taxon/rank/text() = 'kingdom'">

	<tr><td><xsl:value-of select="taxon/name"/></td><td>

<xsl:value-of select="recursive/assembledKmerObservations"/> </td><td>
          	<xsl:value-of select="100* recursive/coloredProportionInRank"/>%
</td></tr>
</xsl:if>

     </xsl:for-each>
</tbody></table>


<table><caption>rank: phylum</caption><tbody>
<tr><th>Taxon</th><th>Colored k-mer observations</th>
<th>Colored proportion</th></tr>

      <xsl:for-each select="root/entry">

        <xsl:sort data-type="number" order="descending" select="recursive/assembledKmerObservations" />

	<xsl:if test="taxon/rank/text() = 'phylum'">

	<tr><td><xsl:value-of select="taxon/name"/></td><td>

<xsl:value-of select="recursive/assembledKmerObservations"/> </td><td>
          	<xsl:value-of select="100* recursive/coloredProportionInRank"/>%
</td></tr>
</xsl:if>

     </xsl:for-each>
</tbody></table>



<table><caption>rank: class</caption><tbody>
<tr><th>Taxon</th><th>Colored k-mer observations</th>
<th>Colored proportion</th></tr>

      <xsl:for-each select="root/entry">

        <xsl:sort data-type="number" order="descending" select="recursive/assembledKmerObservations" />
	<xsl:if test="taxon/rank/text() = 'class'">

	<tr><td><xsl:value-of select="taxon/name"/></td><td>

<xsl:value-of select="recursive/assembledKmerObservations"/> </td><td>
          	<xsl:value-of select="100* recursive/coloredProportionInRank"/>%
</td></tr>
</xsl:if>

     </xsl:for-each>
</tbody></table>




<table><caption>rank: order</caption><tbody>
<tr><th>Taxon</th><th>Colored k-mer observations</th>
<th>Colored proportion</th></tr>

      <xsl:for-each select="root/entry">

        <xsl:sort data-type="number" order="descending" select="recursive/assembledKmerObservations" />
	<xsl:if test="taxon/rank/text() = 'order'">

	<tr><td><xsl:value-of select="taxon/name"/></td><td>

<xsl:value-of select="recursive/assembledKmerObservations"/> </td><td>
          	<xsl:value-of select="100* recursive/coloredProportionInRank"/>%
</td></tr>
</xsl:if>

     </xsl:for-each>
</tbody></table>





<table><caption>rank: family</caption><tbody>
<tr><th>Taxon</th><th>Colored k-mer observations</th>
<th>Colored proportion</th></tr>

      <xsl:for-each select="root/entry">

        <xsl:sort data-type="number" order="descending" select="recursive/assembledKmerObservations" />
	<xsl:if test="taxon/rank/text() = 'family'">

	<tr><td><xsl:value-of select="taxon/name"/></td><td>

<xsl:value-of select="recursive/assembledKmerObservations"/> </td><td>
          	<xsl:value-of select="100* recursive/coloredProportionInRank"/>%
</td></tr>
</xsl:if>

     </xsl:for-each>
</tbody></table>




<table><caption>rank: genus</caption><tbody>
<tr><th>Taxon</th><th>Colored k-mer observations</th>
<th>Colored proportion</th></tr>

      <xsl:for-each select="root/entry">

        <xsl:sort data-type="number" order="descending" select="recursive/assembledKmerObservations" />
	<xsl:if test="taxon/rank/text() = 'genus'">

	<tr><td><xsl:value-of select="taxon/name"/></td><td>

<xsl:value-of select="recursive/assembledKmerObservations"/> </td><td>
          	<xsl:value-of select="100* recursive/coloredProportionInRank"/>%
</td></tr>
</xsl:if>

     </xsl:for-each>
</tbody></table>




<table><caption>rank: species</caption><tbody>
<tr><th>Taxon</th><th>Colored k-mer observations</th>
<th>Colored proportion</th></tr>

      <xsl:for-each select="root/entry">

        <xsl:sort data-type="number" order="descending" select="recursive/assembledKmerObservations" />
	<xsl:if test="taxon/rank/text() = 'species'">

	<tr><td><xsl:value-of select="taxon/name"/></td><td>

<xsl:value-of select="recursive/assembledKmerObservations"/> </td><td>
          	<xsl:value-of select="100* recursive/coloredProportionInRank"/>%
</td></tr>
</xsl:if>

     </xsl:for-each>
</tbody></table>


  </body>
  </html>
</xsl:template>

</xsl:stylesheet> 
